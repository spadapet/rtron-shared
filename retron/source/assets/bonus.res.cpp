#include "pch.h"
#include "bonus.res.h"

namespace res
{
    void register_bonus()
    {
        ff::global_resources::add(::assets::bonus::data());
    }
}
