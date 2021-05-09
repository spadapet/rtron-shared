#pragma once

#include "source/core/game_spec.h"
#include "source/level/collision.h"
#include "source/level/entities.h"
#include "source/level/particles.h"
#include "source/level/position.h"

namespace retron
{
    class level_service;

    enum class level_phase
    {
        ready,
        playing,
        dead,
        won
    };

    class level
    {
    public:
        level(retron::level_service& level_service, retron::level_spec&& level_spec);

        void advance(const ff::rect_fixed& camera_rect);
        void render(ff::dx11_target_base& target, ff::dx11_depth& depth, const ff::rect_fixed& target_rect, const ff::rect_fixed& camera_rect);

        retron::level_phase phase() const;
        size_t phase_counter() const;
        void start(); // move from ready -> playing
        const retron::level_spec& level_spec() const;

    private:
        void init_resources();
        void init_entities();

        ff::rect_fixed bounds_box(entt::entity entity);
        ff::rect_fixed hit_box(entt::entity entity);

        entt::entity create_entity(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_electrode(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_grunt(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_player(size_t index_in_level);
        entt::entity create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir);
        entt::entity create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top);
        entt::entity create_animation(std::shared_ptr<ff::animation_player_base> player, ff::point_fixed pos, bool top);
        entt::entity create_bounds(const ff::rect_fixed& rect);
        entt::entity create_box(const ff::rect_fixed& rect);
        void create_objects(size_t& count, retron::entity_type type, const ff::rect_fixed& bounds, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func);
        void recreate_player(entt::entity entity);

        void advance_entity(entt::entity entity, retron::entity_type type);
        void advance_player(entt::entity entity);
        void advance_player_bullet(entt::entity entity);
        void advance_grunt(entt::entity entity);
        void advance_animation(entt::entity entity);
        void advance_entity_followers();
        void advance_phase();

        void handle_collisions();
        void handle_bounds_collision(entt::entity target_entity, entt::entity level_entity);
        void handle_entity_collision(entt::entity target_entity, entt::entity source_entity);

        void destroy_player_bullet(entt::entity bullet_entity, entt::entity by_entity, retron::entity_box_type by_type);
        void destroy_grunt(entt::entity grunt_entity, entt::entity by_entity, retron::entity_box_type by_type);
        void destroy_obstacle(entt::entity obstacle_entity, entt::entity by_entity, retron::entity_box_type by_type);

        void render_particles(ff::draw_base& draw);
        void render_entity(entt::entity entity, retron::entity_type type, ff::draw_base& draw);
        void render_player(entt::entity entity, ff::draw_base& draw);
        void render_player_bullet(entt::entity entity, ff::draw_base& draw);
        void render_bonus(entt::entity entity, retron::entity_type type, ff::draw_base& draw);
        void render_electrode(entt::entity entity, ff::draw_base& draw);
        void render_hulk(entt::entity entity, ff::draw_base& draw);
        void render_grunt(entt::entity entity, ff::draw_base& draw);
        void render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_base* anim, ff::fixed_int frame);
        void render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_player_base* player);
        void render_debug(ff::draw_base& draw);

        bool player_active() const;
        entt::entity player_target(size_t enemy_index) const;
        void player_add_points(entt::entity player_or_bullet, entt::entity destroyed_entity);

        size_t pick_grunt_move_counter();
        ff::point_fixed pick_move_destination(entt::entity entity, entt::entity destEntity, retron::collision_box_type collision_type);

        void remove_tracked_object(entt::entity entity);
        void enum_entities(const std::function<void(entt::entity, retron::entity_type)>& func);

        enum class internal_phase_t
        {
            init,
            ready,
            show_enemies,
            show_players,
            playing,
            winning,
            won
        };

        void internal_phase(internal_phase_t new_phase);

        retron::level_service& level_service_;
        const retron::game_spec& game_spec_;
        const retron::difficulty_spec& difficulty_spec_;
        retron::level_spec level_spec_;

        entt::registry registry;
        retron::entities entities;
        retron::position position;
        retron::collision collision;
        retron::particles particles;

        std::unordered_map<std::string_view, retron::particles::effect_t> particle_effects;
        std::vector<std::pair<entt::entity, entt::entity>> collisions;
        std::vector<std::pair<entt::entity, retron::entity_type>> sorted_entities;
        std::forward_list<ff::signal_connection> connections;

        std::array<ff::auto_resource<ff::animation_base>, 8> player_walk_anims;
        std::array<ff::auto_resource<ff::animation_base>, 3> electrode_anims;
        std::array<ff::auto_resource<ff::animation_base>, 3> electrode_die_anims;
        ff::auto_resource<ff::animation_base> player_bullet_anim;
        ff::auto_resource<ff::animation_base> grunt_walk_anim;

        internal_phase_t phase_;
        size_t phase_counter_;
        size_t phase_length;
        size_t frame_count;
    };
}
