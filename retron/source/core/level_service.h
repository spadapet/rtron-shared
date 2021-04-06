#pragma once

#include "source/core/game_service.h"

namespace retron
{
    struct level_spec;

    class level_service : public retron::game_service
    {
    public:
        virtual size_t level_index() const = 0;
        virtual const retron::level_spec& level_spec() const = 0;
        virtual size_t player_count() const = 0;
        virtual retron::player& player(size_t index) const = 0;
        virtual retron::player& player_or_coop(size_t index) const = 0;
    };
}
