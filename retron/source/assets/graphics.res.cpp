#include "pch.h"
#include "graphics.res.h"

namespace res
{
    void register_graphics()
    {
        ff::global_resources::add(::assets::graphics::data());
    }
}
