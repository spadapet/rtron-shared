#pragma once

namespace retron
{
    class game_service;
    struct player;

    class level_service
    {
    public:
        virtual const retron::game_service& game_service() const = 0;
        virtual size_t player_count() const = 0;
        virtual const retron::player& player(size_t index) const = 0;
        virtual const retron::player& player_or_coop(size_t index) const = 0;
    };
}
