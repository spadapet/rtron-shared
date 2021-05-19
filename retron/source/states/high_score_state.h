#pragma once

#include "source/core/game_spec.h"
#include "source/core/options.h"

namespace retron
{
    class high_score_state : public ff::state
    {
    public:
        high_score_state(const retron::game_options& game_options, const retron::player& player, std::shared_ptr<ff::state> next_state);

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

    private:
        retron::game_options game_options;
        retron::player player;
        std::shared_ptr<ff::state> next_state;
    };
}
