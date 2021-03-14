#pragma once

#include "source/core/render_targets.h"

namespace retron
{
    class title_page;

    class title_state : public ff::state, public std::enable_shared_from_this<retron::title_state>
    {
    public:
        title_state();

        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

    private:
        retron::render_targets targets;
        Noesis::Ptr<retron::title_page> title_page;
        std::shared_ptr<ff::ui_view_state> view_state;
    };
}
