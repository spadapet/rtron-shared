#include "pch.h"
#include "source/states/debug_state.h"
#include "source/states/ui_view_state.h"

bool retron::debug_state::visible() const
{
    return this->view_state != nullptr;
}

void retron::debug_state::visible(std::shared_ptr<ff::ui_view> view, std::shared_ptr<ff::state> under_state)
{
    this->view_state = std::make_shared<retron::ui_view_state>(view);
    this->under_state = under_state;

    view->focused(true);
}

void retron::debug_state::hide()
{
    this->view_state.reset();
    this->under_state.reset();
}

void retron::debug_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    if (this->visible())
    {
        if (this->under_state)
        {
            this->under_state->render(target, depth);
        }

        ff::state::render(target, depth);
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
