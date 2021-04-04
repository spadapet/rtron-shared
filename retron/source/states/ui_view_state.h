#pragma once

namespace retron
{
    class ui_view_state : public ff::ui_view_state
    {
    public:
        ui_view_state(std::shared_ptr<ff::ui_view> view);

        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;
    };
}
