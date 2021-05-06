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
        virtual void render() override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        // game_service
        virtual const retron::game_options& game_options() const override;
        virtual const retron::difficulty_spec& difficulty_spec() const override;
        virtual const ff::input_event_provider& input_events(const retron::player& player) const override;
        virtual void add_player_points(size_t player_index, size_t points) override;

        // Debug
        void debug_restart_level();

    private:
        void init_resources();
        void init_input();
        void init_players();
        void init_level_states();

        void add_level_state(size_t level_index, std::vector<retron::player*>&& players);
        const retron::level_spec& level_spec(size_t level_index);
        void transition_to_next_level();

        retron::level& level() const;
        retron::level_state& level_state() const;
        retron::player& coop_player();

        retron::game_options game_options_;
        retron::difficulty_spec difficulty_spec_;
        retron::level_set_spec level_set_spec;
        std::forward_list<ff::signal_connection> connections;

        // Input
        std::unique_ptr<ff::input_event_provider> game_input_events;
        std::array<std::unique_ptr<ff::input_event_provider>, constants::MAX_PLAYERS> player_input_events;
        std::array<retron::player, constants::MAX_PLAYERS + 1> players;

        // Level
        std::vector<std::pair<std::shared_ptr<retron::level_state>, std::shared_ptr<ff::state_wrapper>>> level_states;
        size_t playing_level_state;

        // Graphics
        ff::auto_resource<ff::sprite_base> player_life_sprite;
        ff::auto_resource<ff::sprite_font> game_font;
    };
}
