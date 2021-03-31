#pragma once

#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/core/options.h"

namespace retron
{
    class debug_state;

    class app_state : public ff::state, public retron::app_service
    {
    public:
        app_state();
        virtual ~app_state() override;

        // State
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void advance_input() override;
        virtual void frame_rendered(ff::state::advance_t type, ff::dx11_target_base& target, ff::dx11_depth& depth) override;
        virtual void save_settings() override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        virtual retron::audio& audio() override;
        virtual const retron::system_options& system_options() const override;
        virtual const retron::game_options& default_game_options() const override;
        virtual const retron::game_spec& game_spec() const override;
        virtual void system_options(const retron::system_options& options) override;
        virtual void default_game_options(const retron::game_options& options) override;
        virtual ff::palette_base& palette() override;
        virtual ff::palette_base& player_palette(size_t player) override;
        virtual ff::draw_device& draw_device() const override;
        virtual ff::signal_sink<void>& destroyed() override;
        virtual ff::signal_sink<void>& reload_resources_sink() override;
        virtual retron::render_debug_t render_debug() const override;

        double time_scale() const;
        ff::state::advance_t advance_type() const;

    private:
        void init_options();
        void init_resources();
        void init_debug_state();
        void init_game_state();
        void apply_system_options();

        void on_custom_debug();
        void on_resources_rebuilt();
        void on_restart_level();
        void on_restart_game();
        void on_rebuild_resources();

        // Globals
        std::shared_ptr<ff::state_wrapper> game_state;
        retron::system_options system_options_;
        retron::game_options game_options_;
        retron::game_spec game_spec_;

        // Rendering
        std::unique_ptr<ff::draw_device> draw_device_;
        std::array<std::shared_ptr<ff::palette_cycle>, constants::MAX_PLAYERS> player_palettes;
        ff::auto_resource<ff::palette_data> palette_data;
        ff::viewport viewport;

        // Audio
        std::unique_ptr<retron::audio> audio_;

        // Debugging
        std::shared_ptr<retron::debug_state> debug_state;
        ff::auto_resource<ff::input_mapping> debug_input_mapping;
        std::unique_ptr<ff::input_event_provider> debug_input_events;
        std::forward_list<ff::signal_connection> connections;
        ff::signal<void> destroyed_signal;
        ff::signal<void> reload_resources_signal;
        double debug_time_scale;
        bool debug_stepping_frames;
        bool debug_step_one_frame;
        bool rebulding_resources;
        retron::render_debug_t render_debug_;
    };
}
