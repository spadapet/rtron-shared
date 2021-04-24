#include "pch.h"
#include "sprites.res.h"

namespace res
{
    void register_sprites()
    {
        ff::global_resources::add(::assets::sprites::data());
    }
}
