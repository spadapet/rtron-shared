#include "pch.h"
#include "source/states/title_state.h"
#include "source/states/transition_state.h"
#include "source/states/ui_view_state.h"
#include "source/ui/title_page.xaml.h"

retron::title_state::title_state()
{
    this->title_page = *new retron::title_page();

    std::shared_ptr<ff::ui_view> view = std::make_shared<ff::ui_view>(this->title_page);
    this->view_state = std::make_shared<retron::ui_view_state>(view);
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

size_t retron::title_state::child_state_count()
{
    return 1;
}

ff::state* retron::title_state::child_state(size_t index)
{
    return this->view_state.get();
}
