#include "pch.h"
#include "game_spec.res.h"

namespace res
{
    void register_game_spec()
    {
        ff::global_resources::add(::assets::game_spec::data());
    }
}
