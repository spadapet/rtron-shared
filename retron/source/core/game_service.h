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
        virtual void player_add_points(size_t player_index, size_t points) = 0;
    };
}
