#pragma once

namespace retron
{
    class audio;
    struct game_options;
    struct system_options;
    struct game_spec;

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

        // Debug
        virtual ff::signal_sink<void>& destroyed() = 0;
        virtual ff::signal_sink<void>& reload_resources_sink() = 0;
        virtual bool render_debug() const = 0;
    };
}
