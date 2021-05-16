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
        virtual void player_add_points(const retron::player& level_player, size_t points) override;
        virtual bool player_get_life(const retron::player& level_player) override;

        // Debug
        void debug_restart_level();

    private:
        void init_resources();
        void init_input();
        void init_players();
        void init_level_states();

        void render_points_and_lives(ff::draw_base& draw);
        void render_overlay_text(ff::draw_base& draw);

        std::shared_ptr<ff::state> handle_level_ready();
        std::shared_ptr<ff::state> handle_level_won();
        std::shared_ptr<ff::state> handle_level_dead();

        struct playing_level_state
        {
            retron::player& player_or_coop();

            std::vector<retron::player*> players;
            std::shared_ptr<retron::level> level;
            std::shared_ptr<ff::state> state;
        };

        playing_level_state create_level_state(size_t level_index, const std::vector<retron::player*>& players);
        const retron::level_spec& level_spec(size_t level_index);
        playing_level_state& playing();
        const playing_level_state& playing() const;
        retron::player& coop_player();

        retron::game_options game_options_;
        retron::difficulty_spec difficulty_spec_;
        retron::level_set_spec level_set_spec;
        std::forward_list<ff::signal_connection> connections;

        // Input
        std::unique_ptr<ff::input_event_provider> game_input_events;
        std::array<std::unique_ptr<ff::input_event_provider>, constants::MAX_PLAYERS> player_input_events;
        std::array<retron::player, constants::MAX_PLAYERS + 1> players;

        // Playing
        std::vector<playing_level_state> playing_states;
        size_t playing_index;

        // Graphics
        ff::auto_resource<ff::sprite_base> player_life_sprite;
        ff::auto_resource<ff::sprite_font> game_font;
    };
}
