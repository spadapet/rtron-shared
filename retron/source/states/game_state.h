#pragma once

#include "source/core/options.h"
#include "source/core/game_service.h"
#include "source/core/game_spec.h"

namespace retron
{
    class game_state : public ff::state, public retron::game_service
    {
    public:
        game_state();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        // game_service
        virtual const retron::game_options& game_options() const override;
        virtual const retron::difficulty_spec& difficulty_spec() const override;
        virtual const ff::input_event_provider& input_events(const retron::player& player) override;

        // Debug
        void restart_level();

    private:
        void init_input();
        void init_players();
        void init_level();
        retron::player& coop_player();
        const retron::level_spec& level_spec(size_t index) const;

        retron::game_options game_options_;
        retron::difficulty_spec difficulty_spec_;
        retron::level_set_spec level_set_spec;

        // Input
        ff::auto_resource<ff::input_mapping> game_input_mapping;
        std::unique_ptr<ff::input_event_provider> game_input_events;
        std::array<ff::auto_resource<ff::input_mapping>, constants::MAX_PLAYERS> player_input_mappings;
        std::array<std::unique_ptr<ff::input_event_provider>, constants::MAX_PLAYERS> player_input_events;
        std::array<retron::player, constants::MAX_PLAYERS + 1> players;

        // Level
        std::vector<std::shared_ptr<ff::state_wrapper>> level_states;
        size_t playing_level_state;
    };
}
