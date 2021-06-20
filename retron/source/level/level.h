#pragma once

#include "source/core/game_spec.h"
#include "source/core/level_base.h"
#include "source/core/particles.h"
#include "source/level/collision.h"
#include "source/level/entities.h"
#include "source/level/level_logic.h"
#include "source/level/level_render.h"
#include "source/level/position.h"

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
    class game_service;

    class level
        : public retron::level_base
        , public retron::level_logic_host
        , public retron::level_render_host
        , public ff::state
    {
    public:
        level(retron::game_service& game_service, const retron::level_spec& level_spec, const std::vector<const retron::player*>& players);

        // ff::state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

        // retron::level_base
        virtual retron::level_phase phase() const override;
        virtual void start() override;
        virtual void restart() override;
        virtual void stop() override;
        virtual const std::vector<const retron::player*>& players() const override;

        // retron::level_logic_host, retron::level_render_host
        virtual entt::registry& host_registry() override;
        virtual const entt::registry& host_registry() const override;
        virtual const retron::difficulty_spec& host_difficulty_spec() const override;
        virtual size_t host_frame_count() const override;
        virtual void host_create_particles(std::string_view name, const ff::point_fixed& pos) override;
        virtual void host_create_bullet(entt::entity player_entity, ff::point_fixed shot_vector) override;
        virtual void host_handle_dead_player(entt::entity entity, const retron::player& player) override;

    private:
        void init_resources();
        void init_entities();

        ff::rect_fixed bounds_box(entt::entity entity);

        entt::entity create_entity(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_bonus(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_electrode(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_grunt(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_hulk(retron::entity_type type, const ff::point_fixed& pos, size_t group);
        entt::entity create_player(const retron::player& player);
        entt::entity create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir);
        entt::entity create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top);
        entt::entity create_animation(std::shared_ptr<ff::animation_player_base> player, ff::point_fixed pos, bool top);
        entt::entity create_bounds(const ff::rect_fixed& rect);
        entt::entity create_box(const ff::rect_fixed& rect);
        void create_start_particles(entt::entity entity);
        void create_objects(size_t& count, retron::entity_type type, const ff::rect_fixed& bounds, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func);

        void advance_entities();
        void advance_entity_followers();
        void advance_phase();

        void handle_particle_effect_done(int effect_id);
        void handle_entity_created(entt::entity entity);
        void handle_entity_deleted(entt::entity entity);
        void handle_collisions();
        void handle_bounds_collision(entt::entity target_entity, entt::entity level_entity);
        void handle_entity_collision(entt::entity target_entity, entt::entity source_entity);

        void destroy_player_bullet(entt::entity bullet_entity, entt::entity by_entity);
        void destroy_enemy(entt::entity entity, entt::entity by_entity);
        void destroy_electrode(entt::entity obstacle_entity, entt::entity by_entity);
        void destroy_bonus(entt::entity bonus_entity, entt::entity by_entity);
        void push_hulk(entt::entity enemy_entity, entt::entity by_entity);

        void render_particles(ff::draw_base& draw);
        void render_debug(ff::draw_base& draw);

        bool player_active() const;
        entt::entity player_target(size_t enemy_index) const;
        void player_add_points(entt::entity player_or_bullet, entt::entity destroyed_entity);

        size_t pick_grunt_move_frame();
        ff::point_fixed pick_move_destination(entt::entity entity, entt::entity dest_entity, retron::collision_box_type collision_type);
        entt::entity pick_hulk_target(entt::entity entity);

        enum class internal_phase_t
        {
            init,
            ready,
            before_show,
            show_enemies,
            show_players,
            playing,
            winning,
            won,
            game_over
        };

        void internal_phase(internal_phase_t new_phase);

        retron::game_service& game_service;
        const retron::difficulty_spec& difficulty_spec_;
        retron::level_spec level_spec_;
        std::vector<const retron::player*> players_;

        entt::registry registry;
        retron::entities entities;
        retron::position position;
        retron::collision collision;
        retron::particles particles;
        retron::level_logic level_logic;
        retron::level_render level_render;

        std::unordered_map<std::string_view, retron::particles::effect_t> particle_effects;
        std::vector<std::pair<entt::entity, entt::entity>> collisions;
        std::forward_list<ff::signal_connection> connections;

        std::array<ff::auto_resource<ff::animation_base>, 3> electrode_die_anims;
        std::array<ff::auto_resource<ff::animation_base>, static_cast<size_t>(retron::bonus_type::count)> bonus_die_anims;

        internal_phase_t phase_;
        size_t phase_counter;
        size_t frame_count;
        size_t bonus_collected;
    };
}
