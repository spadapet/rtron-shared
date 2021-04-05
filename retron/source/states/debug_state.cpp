#include "pch.h"
#include "source/states/debug_state.h"

bool retron::debug_state::visible()
{
    return this->top_state != nullptr;
}

void retron::debug_state::visible(std::shared_ptr<ff::state> top_state, std::shared_ptr<ff::state> under_state)
{
    this->top_state = std::make_shared<ff::state_wrapper>(top_state);
    this->under_state = under_state;
}

void retron::debug_state::hide()
{
    this->top_state.reset();
    this->under_state.reset();
}

void retron::debug_state::render()
{
    if (this->visible())
    {
        if (this->under_state)
        {
            this->under_state->render();
        }

        ff::state::render();
    }
}

size_t retron::debug_state::child_state_count()
{
    return this->visible() ? 1 : 0;
}

ff::state* retron::debug_state::child_state(size_t index)
{
    return this->top_state.get();
}
