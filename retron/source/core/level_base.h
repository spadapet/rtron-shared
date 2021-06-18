#pragma once

namespace retron
{
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
        virtual retron::level_phase phase() const = 0;
        virtual void start() = 0; // move from ready->playing
        virtual void restart() = 0; // move from dead->ready
        virtual void stop() = 0; // move from dead->game_over
        virtual const std::vector<const retron::player*>& players() const = 0;
    };

    class level_logic_base
    {
    public:
        virtual void advance_time(retron::entity_category categories, entt::registry& registry, const retron::difficulty_spec& difficulty_spec, size_t frame_count) = 0;
    };

    class level_render_base
    {
    public:
        virtual void render(ff::draw_base& draw, const entt::registry& registry, const retron::difficulty_spec& difficulty_spec, size_t frame_count) = 0;
    };
}
