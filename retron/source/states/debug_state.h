#pragma once

#include "source/core/render_targets.h"

namespace retron
{
    class debug_page;
    class debug_page_view_model;

    class debug_state : public ff::state
    {
    public:
        debug_state();
        virtual ~debug_state() override;

        bool visible() const;
        void visible(std::shared_ptr<ff::state> under_state);
        void hide();

        // State
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

        // UI events
        ff::signal_sink<void>& restart_level_event();
        ff::signal_sink<void>& restart_game_event();
        ff::signal_sink<void>& rebuild_resources_event();

    private:
        friend class retron::debug_page_view_model;

        retron::render_targets targets;
        Noesis::Ptr<retron::debug_page> debug_page;
        std::shared_ptr<ff::ui_view_state> view_state;
        std::shared_ptr<ff::state> under_state;
        ff::signal<void> restart_level_event_;
        ff::signal<void> restart_game_event_;
        ff::signal<void> rebuild_resources_event_;
    };
}
