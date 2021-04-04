#include "pch.h"
#include "source/states/particle_lab_state.h"

retron::particle_lab_state::particle_lab_state()
{}

std::shared_ptr<ff::state> retron::particle_lab_state::advance_time()
{
    return nullptr;
}

void retron::particle_lab_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
}
