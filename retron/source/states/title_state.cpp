#include "pch.h"
#include "source/states/title_state.h"
#include "source/states/transition_state.h"
#include "source/ui/title_page.xaml.h"

retron::title_state::title_state()
    : targets(retron::render_target_types::rgb_pma_2)
{
    this->title_page = *new retron::title_page();

    std::shared_ptr<ff::ui_view> view = std::make_shared<ff::ui_view>(this->title_page);
    this->view_state = std::make_shared<ff::ui_view_state>(view, this->targets.target(retron::render_target_types::rgb_pma_2), this->targets.depth(retron::render_target_types::rgb_pma_2));
}

std::shared_ptr<ff::state> retron::title_state::advance_time()
{
    ff::state::advance_time();

    std::shared_ptr<ff::state> new_state = this->title_page->view_model()->pending_state();
    if (new_state)
    {
        new_state = std::make_shared<retron::transition_state>(this->shared_from_this(), new_state, "transition_bg_2.png");
    }

    return new_state;
}

void retron::title_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    this->targets.clear();

    ff::state::render(target, depth);

    this->targets.render(target);
}

size_t retron::title_state::child_state_count()
{
    return 1;
}

ff::state* retron::title_state::child_state(size_t index)
{
    return this->view_state.get();
}
