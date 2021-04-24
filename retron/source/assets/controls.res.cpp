#include "pch.h"
#include "controls.res.h"

namespace res
{
    void register_controls()
    {
        ff::global_resources::add(::assets::controls::data());
    }
}
