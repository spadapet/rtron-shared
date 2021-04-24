#include "pch.h"
#include "player.res.h"

namespace res
{
    void register_player()
    {
        ff::global_resources::add(::assets::player::data());
    }
}
