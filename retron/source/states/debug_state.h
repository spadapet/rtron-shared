#pragma once

namespace retron
{
    class ui_view_state;

    class debug_state : public ff::state
    {
    public:
        bool visible() const;
        void visible(std::shared_ptr<ff::ui_view> view, std::shared_ptr<ff::state> under_state);
        void hide();

        // State
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

    private:
        std::shared_ptr<retron::ui_view_state> view_state;
        std::shared_ptr<ff::state> under_state;
    };
}
