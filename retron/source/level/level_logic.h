#pragma once

#include "source/core/level_base.h"

namespace retron::comp
{
    struct position;
    struct velocity;

    struct player;
    struct grunt;
    struct hulk;
    struct bonus;
    struct animation;
}

namespace retron
{
    class collision;

    class level_logic : public level_logic_base
    {
    public:
        level_logic(level_logic_host& host, retron::collision& collision);

        virtual void advance_time(retron::entity_category categories) override;
        virtual void reset() override;

    private:
        void advance_player(entt::entity entity, retron::comp::player& comp, const retron::comp::position& pos, const retron::comp::velocity& vel);
        void advance_grunt(entt::entity entity, retron::comp::grunt& comp, const retron::comp::position& pos);
        void advance_hulk(entt::entity entity, retron::comp::hulk& comp, const retron::comp::position& pos, const retron::comp::velocity& vel);
        void advance_bonus(entt::entity entity, retron::comp::bonus& comp, const retron::comp::position& pos, const retron::comp::velocity& vel);
        void advance_animation(entt::entity entity, retron::comp::animation& comp, const retron::comp::position& pos);

        size_t pick_grunt_move_frame() const;
        ff::point_fixed pick_grunt_move_destination(entt::entity entity, entt::entity dest_entity) const;
        entt::entity pick_grunt_player_target(size_t enemy_index) const;
        entt::entity pick_hulk_target(entt::entity entity) const;

        retron::level_logic_host& host;
        retron::collision& collision;
        std::vector<size_t> next_hulk_group_turn;
    };
}
