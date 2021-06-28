#pragma once

#include "source/core/game_spec.h"
#include "source/core/level_base.h"
#include "source/core/particles.h"
#include "source/level/collision.h"
#include "source/level/entities.h"
#include "source/level/level_logic.h"
#include "source/level/level_collision_logic.h"
#include "source/level/level_render.h"

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
        virtual void host_create_particles(std::string_view name, const ff::point_fixed& pos, const retron::particle_effect_options* options = nullptr) override;
        virtual void host_create_bullet(entt::entity player_entity, ff::point_fixed shot_vector) override;
        virtual void host_handle_dead_player(entt::entity entity, const retron::player& player) override;
        virtual void host_add_points(const retron::player& player, size_t points) override;

    private:
        void init_resources();
        void init_entities();

        entt::entity create_player(const retron::player& player);
        void create_start_particles(entt::entity entity);
        void create_objects(size_t& count, retron::entity_type type, const ff::rect_fixed& bounds, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func);

        void advance_entities();
        void advance_particle_positions();
        void advance_phase();

        void handle_particle_effect_done(int effect_id);
        void handle_entity_created(entt::registry& registry, entt::entity entity);
        void handle_tracked_entity_deleted(entt::registry& registry, entt::entity entity);

        void render_particles(ff::draw_base& draw);
        void render_debug(ff::draw_base& draw);

        bool player_active() const;

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
        retron::collision collision;
        retron::particles particles;
        retron::level_logic level_logic;
        retron::level_collision_logic level_collision_logic;
        retron::level_render level_render;

        std::forward_list<entt::scoped_connection> connections;
        std::forward_list<ff::signal_connection> ff_connections;

        internal_phase_t phase_;
        size_t phase_counter;
        size_t frame_count;
    };
}
