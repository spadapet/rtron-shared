#pragma once

#include "source/level/particles.h"

namespace retron
{
    class particle_lab_state : public ff::state
    {
    public:
        particle_lab_state();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;

    private:
        retron::particles particles;
    };
}
