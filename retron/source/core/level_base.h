#pragma once

namespace retron
{
    class collision;
    enum class entity_category;
    struct difficulty_spec;

    enum class level_phase
    {
        ready,
        playing,
        won,
        dead,
        game_over,
    };

    class level_base
    {
    public:
        virtual ~level_base() = default;

        virtual retron::level_phase phase() const = 0;
        virtual void start() = 0; // move from ready->playing
        virtual void restart() = 0; // move from dead->ready
        virtual void stop() = 0; // move from dead->game_over
        virtual const std::vector<const retron::player*>& players() const = 0;
    };

    class level_logic_base
    {
    public:
        virtual ~level_logic_base() = default;

        virtual void advance_time(retron::entity_category categories) = 0;
        virtual void reset() = 0;
    };

    class level_logic_host
    {
    public:
        virtual ~level_logic_host() = default;

        virtual entt::registry& host_registry() = 0;
        virtual const retron::difficulty_spec& host_difficulty_spec() const = 0;
        virtual size_t host_frame_count() const = 0;
        virtual void host_create_particles(std::string_view name, const ff::point_fixed& pos) = 0;
        virtual void host_create_bullet(entt::entity player_entity, ff::point_fixed shot_vector) = 0;
        virtual void host_handle_dead_player(entt::entity entity, const retron::player& player) = 0;
    };

    class level_render_base
    {
    public:
        virtual ~level_render_base() = default;

        virtual void render(ff::draw_base& draw) = 0;
    };

    class level_render_host
    {
    public:
        virtual ~level_render_host() = default;

        virtual const entt::registry& host_registry() const = 0;
        virtual const retron::difficulty_spec& host_difficulty_spec() const = 0;
        virtual size_t host_frame_count() const = 0;
    };
}
