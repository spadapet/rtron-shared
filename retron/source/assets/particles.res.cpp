#include "pch.h"
#include "particles.res.h"

namespace res
{
    void register_particles()
    {
        ff::global_resources::add(::assets::particles::data());
    }
}
