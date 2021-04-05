#pragma once

#include "source/level/particles.h"

namespace retron
{
    class particle_lab_state : public ff::state
    {
    public:
        particle_lab_state(std::shared_ptr<ff::ui_view> view);

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

    private:
        void on_page_destroyed();
        void on_mouse_click(int button, ff::point_float pos, std::string_view name, retron::particles::effect_t& effect);

        std::shared_ptr<ff::ui_view> view;
        std::forward_list<ff::signal_connection> connections;
        retron::particles particles;
    };
}
