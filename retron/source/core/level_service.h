#pragma once

namespace retron
{
    class game_service;
    struct level_spec;
    struct player;

    class level_service
    {
    public:
        virtual const retron::game_service& game_service() const = 0;
        virtual size_t level_index() const = 0;
        virtual const retron::level_spec& level_spec() const = 0;
        virtual size_t player_count() const = 0;
        virtual const retron::player& player(size_t index) const = 0;
        virtual const retron::player& player_or_coop(size_t index) const = 0;
    };
}
