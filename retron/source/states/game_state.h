#pragma once

#include "source/core/options.h"
#include "source/core/game_service.h"
#include "source/core/game_spec.h"

namespace retron
{
    class level;
    class level_state;

    class game_state : public ff::state, public retron::game_service
    {
    public:
        game_state();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        // game_service
        virtual const retron::game_options& game_options() const override;
        virtual const retron::difficulty_spec& difficulty_spec() const override;
        virtual const ff::input_event_provider& input_events(const retron::player& player) const override;
        virtual void player_add_points(const retron::player& level_player, size_t points) override;
        virtual bool player_take_life(const retron::player& level_player) override;

        // Debug
        void debug_restart_level();

    private:
        void init_input();
        void init_players();
        void init_playing_states();

        struct playing_t
        {
            retron::player& player_or_coop() const;

            std::vector<retron::player*> players;
            std::shared_ptr<retron::level> level;
            std::shared_ptr<ff::state_wrapper> state;
        };

        void handle_level_ready(playing_t& playing);
        void handle_level_won(playing_t& playing);
        void handle_level_dead(playing_t& playing);
        std::shared_ptr<ff::state> handle_game_over(playing_t& playing);

        const retron::level_spec& level_spec(size_t level_index);
        playing_t create_playing(const std::vector<retron::player*>& players);

        retron::game_options game_options_;
        retron::difficulty_spec difficulty_spec_;
        retron::level_set_spec level_set_spec;

        // Input
        std::unique_ptr<ff::input_event_provider> game_input_events;
        std::array<std::unique_ptr<ff::input_event_provider>, constants::MAX_PLAYERS> player_input_events;
        std::array<retron::player, constants::MAX_PLAYERS + 1> players;

        // Playing
        std::vector<playing_t> playing_states;
        size_t playing_index;
    };
}
