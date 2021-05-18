#pragma once

namespace retron
{
    struct difficulty_spec;
    struct game_options;
    struct player;

    class game_service
    {
    public:
        virtual ~game_service() = default;

        virtual const retron::game_options& game_options() const = 0;
        virtual const retron::difficulty_spec& difficulty_spec() const = 0;
        virtual const ff::input_event_provider& input_events(const retron::player& player) const = 0;
        virtual void player_add_points(const retron::player& player, size_t points) = 0;
        virtual bool player_add_life(const retron::player& player) = 0;
    };
}
