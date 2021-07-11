#pragma once

#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/core/render_targets.h"
#include "source/core/options.h"
#include "source/core/particles.h"

namespace retron
{
    class debug_state;

    class app_state : public ff::state, public retron::app_service
    {
    public:
        app_state();
        virtual ~app_state() override;

        // ff::state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void advance_input() override;
        virtual void render(ff::target_base& target, ff::depth& depth) override;
        virtual void frame_rendered(ff::state::advance_t type, ff::target_base& target, ff::depth& depth) override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        // retron::app_service
        virtual retron::audio& audio() override;
        virtual const retron::particle_effect_base* level_particle_effect(std::string_view name) override;
        virtual const retron::system_options& system_options() const override;
        virtual const retron::game_options& default_game_options() const override;
        virtual const retron::game_spec& game_spec() const override;
        virtual void system_options(const retron::system_options& options) override;
        virtual void default_game_options(const retron::game_options& options) override;
        virtual ff::palette_base& palette() override;
        virtual ff::palette_base& player_palette(size_t player) override;
        virtual ff::draw_device& draw_device() const override;
        virtual retron::render_targets* render_targets() const override;
        virtual void push_render_targets(retron::render_targets& targets) override;
        virtual void pop_render_targets(ff::target_base& final_target) override;
        virtual ff::signal_sink<>& reload_resources_sink() override;
        virtual retron::render_debug_t render_debug() const override;
        virtual void render_debug(retron::render_debug_t flags) override;
        virtual retron::debug_cheats_t debug_cheats() const override;
        virtual void debug_cheats(retron::debug_cheats_t flags) override;
        virtual void debug_command(size_t command_id) override;

        double time_scale() const;
        ff::state::advance_t advance_type() const;

    private:
        void init_options();
        void init_resources();
        void init_game_state();
        void apply_system_options();
        void save_settings();

        void on_custom_debug();
        void on_resources_rebuilt();

        // Globals
        std::shared_ptr<ff::state> game_state;
        retron::system_options system_options_;
        retron::game_options game_options_;
        retron::game_spec game_spec_;

        // Rendering
        retron::render_targets render_targets_;
        std::vector<retron::render_targets*> render_targets_stack;
        std::shared_ptr<ff::texture> texture_1080;
        std::shared_ptr<ff::target_base> target_1080;
        std::unique_ptr<ff::draw_device> draw_device_;
        std::array<std::shared_ptr<ff::palette_cycle>, constants::MAX_PLAYERS> player_palettes;
        ff::auto_resource<ff::palette_data> palette_data;
        ff::viewport viewport;

        // Resources
        std::unique_ptr<retron::audio> audio_;
        std::unordered_map<std::string_view, retron::particles::effect_t> level_particle_effects;
        ff::auto_resource_value level_particle_value;;

        // Debugging
        std::forward_list<ff::signal_connection> connections;
        std::shared_ptr<retron::debug_state> debug_state;
        std::unique_ptr<ff::input_event_provider> debug_input_events;
        ff::auto_resource<ff::input_mapping> debug_input_mapping;
        ff::signal<> reload_resources_signal;
        retron::render_debug_t render_debug_;
        retron::debug_cheats_t debug_cheats_;
        double debug_time_scale;
        bool debug_stepping_frames;
        bool debug_step_one_frame;
        bool rebuilding_resources_;
        bool pending_hide_debug_state;
    };
}
