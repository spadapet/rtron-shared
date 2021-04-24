#include "pch.h"
#include "electrode.res.h"

namespace res
{
    void register_electrode()
    {
        ff::global_resources::add(::assets::electrode::data());
    }
}
