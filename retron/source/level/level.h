#pragma once

#include "source/core/game_spec.h"
#include "source/level/collision.h"
#include "source/level/entities.h"
#include "source/level/particles.h"
#include "source/level/position.h"

namespace retron
{
    class level_service;

    class level
    {
    public:
        level(const retron::level_service& levelService);

        void advance(const ff::rect_fixed& camera_rect);
        void render(ff::dx11_target_base& target, ff::dx11_depth& depth, const ff::rect_fixed& target_rect, const ff::rect_fixed& camera_rect);

    private:
        void app_service_destroyed();
        void init_resources();

        entt::entity create_entity(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_player(size_t index_in_level);
        entt::entity create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir);
        entt::entity create_bounds(const ff::rect_fixed& rect);
        entt::entity create_box(const ff::rect_fixed& rect);
        void create_objects(size_t count, retron::entity_type type, const ff::rect_fixed& rect, const std::vector<ff::rect_fixed>& avoid_rects);

        void advance_entity(entt::entity entity, retron::entity_type type);
        void advance_player(entt::entity entity);
        void advance_player_bullet(entt::entity entity);
        void advance_grunt(entt::entity entity);
        void advance_particle_effect_positions();

        void handle_collisions();
        void handle_bounds_collision(entt::entity entity1, entt::entity entity2);
        void handle_player_bullet_bounds_collision(entt::entity entity);
        void handle_entity_collision(entt::entity entity1, entt::entity entity2);

        void render_particles(ff::draw_base& draw);
        void render_entity(entt::entity entity, retron::entity_type type, ff::draw_base* draw);
        void render_player(entt::entity entity, ff::draw_base& draw);
        void render_player_bullet(entt::entity entity, ff::draw_base& draw);
        void render_bonus(entt::entity entity, retron::entity_type type, ff::draw_base& draw);
        void render_electrode(entt::entity entity, ff::draw_base& draw);
        void render_hulk(entt::entity entity, ff::draw_base& draw);
        void render_grunt(entt::entity entity, ff::draw_base& draw);
        void render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_base* anim, ff::fixed_int frame);
        void render_debug(ff::draw_base& draw);

        size_t pick_grunt_move_counter();
        ff::point_fixed pick_move_destination(entt::entity entity, entt::entity destEntity, retron::collision_box_type collision_type);

        void enum_entities(const std::function<void(entt::entity, retron::entity_type)>& func);

        const retron::level_service& level_service_;
        const retron::game_spec& game_spec_;
        const retron::level_spec& level_spec_;
        const retron::difficulty_spec& difficulty_spec_;

        entt::registry registry;
        retron::entities entities;
        retron::position position;
        retron::collision collision;
        retron::particles particles;

        std::unordered_map<std::string_view, retron::particles::effect_t> particle_effects;
        std::vector<std::pair<entt::entity, entt::entity>> collisions;
        std::forward_list<ff::signal_connection> connections;

        std::array<ff::auto_resource<ff::animation_base>, 8> player_walk_anims;
        ff::auto_resource<ff::animation_base> player_bullet_anim;

        size_t frame_count;
    };
}
