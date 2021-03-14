#include "pch.h"
#include "source/core/app_service.h"
#include "source/states/debug_state.h"
#include "source/ui/debug_page.xaml.h"

retron::debug_state::debug_state()
    : targets(retron::render_target_types::rgb_pma_2)
{
    this->debug_page = *new retron::debug_page(this);

    std::shared_ptr<ff::ui_view> view = std::make_shared<ff::ui_view>(this->debug_page);
    this->view_state = std::make_shared<ff::ui_view_state>(view, this->targets.target(retron::render_target_types::rgb_pma_2), this->targets.depth(retron::render_target_types::rgb_pma_2));
}

retron::debug_state::~debug_state()
{}

bool retron::debug_state::visible() const
{
    return this->under_state != nullptr;
}

void retron::debug_state::visible(std::shared_ptr<ff::state> under_state)
{
    this->under_state = under_state;

    if (under_state)
    {
        this->view_state->view()->focused(true);
    }
}

void retron::debug_state::hide()
{
    this->under_state.reset();
}

void retron::debug_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    if (this->visible())
    {
        this->under_state->render(target, depth);
        this->targets.clear();

        ff::state::render(target, depth);

        this->targets.render(target);
    }
}

size_t retron::debug_state::child_state_count()
{
    return this->visible() ? 1 : 0;
}

ff::state* retron::debug_state::child_state(size_t index)
{
    return this->view_state.get();
}

ff::signal_sink<void>& retron::debug_state::restart_level_event()
{
    return this->restart_level_event_;
}

ff::signal_sink<void>& retron::debug_state::restart_game_event()
{
    return this->restart_game_event_;
}

ff::signal_sink<void>& retron::debug_state::rebuild_resources_event()
{
    return this->rebuild_resources_event_;
}
