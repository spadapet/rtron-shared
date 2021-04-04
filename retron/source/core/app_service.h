#pragma once

namespace retron
{
    class audio;
    class render_targets;
    struct game_options;
    struct system_options;
    struct game_spec;

    enum class render_debug_t
    {
        none = 0,
        controls = 0x01,
        ai_lines = 0x02,
        collision = 0x04,
        position = 0x08,

        set_0 = none,
        set_1 = controls,
        set_2 = ai_lines | collision | position,
    };

    class app_service
    {
    public:
        virtual ~app_service() = default;

        static app_service& get();

        // Globals
        virtual retron::audio& audio() = 0;

        // Options
        virtual const retron::system_options& system_options() const = 0;
        virtual const retron::game_options& default_game_options() const = 0;
        virtual const retron::game_spec& game_spec() const = 0;
        virtual void system_options(const retron::system_options& options) = 0;
        virtual void default_game_options(const retron::game_options& options) = 0;

        // Rendering
        virtual ff::palette_base& palette() = 0;
        virtual ff::palette_base& player_palette(size_t player) = 0;
        virtual ff::draw_device& draw_device() const = 0;
        virtual retron::render_targets* render_targets() const = 0;
        virtual void push_render_targets(retron::render_targets& targets) = 0;
        virtual void pop_render_targets(ff::dx11_target_base& final_target) = 0;

        // Debug
        virtual ff::signal_sink<void>& destroyed() = 0;
        virtual ff::signal_sink<void>& reload_resources_sink() = 0;
        virtual bool rebuilding_resources() const = 0;
        virtual retron::render_debug_t render_debug() const = 0;
        virtual void render_debug(retron::render_debug_t flags) = 0;
        virtual void debug_command(size_t command_id) = 0;
    };
}
