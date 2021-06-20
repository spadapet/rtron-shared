#pragma once

#include "source/core/game_spec.h"
#include "source/core/level_base.h"

namespace retron
{
    class entities;
    class position;
    class collision;

    class level_collision_logic : public retron::level_collision_logic_base
    {
    public:
        level_collision_logic(retron::level_logic_host& host, retron::entities& entities, retron::position& position, retron::collision& collision);

        virtual void handle_collisions() override;
        virtual void reset() override;

    private:
        void init_resources();
        ff::rect_fixed bounds_box(entt::entity entity);

        void handle_bounds_collision(entt::entity target_entity, entt::entity level_entity);
        void handle_entity_collision(entt::entity target_entity, entt::entity source_entity);

        void destroy_bullet(entt::entity bullet_entity, entt::entity by_entity);
        void destroy_enemy(entt::entity entity, entt::entity by_entity);
        void destroy_electrode(entt::entity electrode_entity, entt::entity by_entity);
        void destroy_bonus(entt::entity bonus_entity, entt::entity by_entity);
        void push_hulk(entt::entity enemy_entity, entt::entity by_entity);
        void add_points(entt::entity player_or_bullet_entity, entt::entity destroyed_entity);

        retron::level_logic_host& host;
        retron::entities& entities;
        retron::position& position;
        retron::collision& collision;
        std::forward_list<ff::signal_connection> connections;

        std::vector<std::pair<entt::entity, entt::entity>> collisions;
        std::array<ff::auto_resource<ff::animation_base>, 3> electrode_die_anims;
        std::array<ff::auto_resource<ff::animation_base>, static_cast<size_t>(retron::bonus_type::count)> bonus_die_anims;

        size_t bonus_collected;
    };
}
