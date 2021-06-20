#pragma once

#include "source/core/game_spec.h"
#include "source/core/level_base.h"

namespace retron
{
    class level_render : public retron::level_render_base
    {
    public:
        level_render(retron::level_render_host& host);

        virtual void render(ff::draw_base& draw) override;

    private:
        void init_resources();

        retron::level_render_host& host;
        std::forward_list<ff::signal_connection> connections;

        std::array<ff::auto_resource<ff::animation_base>, 8> player_walk_anims;
        std::array<ff::auto_resource<ff::animation_base>, 3> electrode_anims;
        std::array<ff::auto_resource<ff::animation_base>, static_cast<size_t>(retron::bonus_type::count)> bonus_anims;
        ff::auto_resource<ff::animation_base> player_bullet_anim;
        ff::auto_resource<ff::animation_base> grunt_walk_anim;
        ff::auto_resource<ff::animation_base> hulk_walk_anim;
    };
}
