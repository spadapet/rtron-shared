#pragma once

#include "source/core/level_base.h"

namespace retron
{
    class level_logic : public level_logic_base
    {
    public:
        level_logic();

        virtual void advance_time(retron::entity_category categories, entt::registry& registry, const retron::difficulty_spec& difficulty_spec, size_t frame_count) override;

    private:
        void advance_player(entt::registry& registry, entt::entity entity, retron::comp::player& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count);
        void advance_grunt(entt::registry& registry, entt::entity entity, retron::comp::grunt& comp, const retron::comp::position& pos, const retron::difficulty_spec& difficulty_spec, size_t frame_count);
        void advance_hulk(entt::registry& registry, entt::entity entity, retron::comp::hulk& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count);
        void advance_bonus(entt::registry& registry, entt::entity entity, retron::comp::bonus& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count);
        void advance_animation(entt::registry& registry, entt::entity entity, retron::comp::animation& comp, const retron::comp::position& pos);

        size_t pick_grunt_move_frame(const retron::difficulty_spec& difficulty_spec, size_t frame_count);
    };
}
