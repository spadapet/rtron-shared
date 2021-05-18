#include "pch.h"
#include "source/states/game_over_state.h"

retron::game_over_state::game_over_state()
{}

std::shared_ptr<ff::state> retron::game_over_state::advance_time()
{
    return nullptr;
}
