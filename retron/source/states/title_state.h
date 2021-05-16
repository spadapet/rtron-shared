#pragma once

namespace retron
{
    class title_page;
    class ui_view_state;

    class title_state : public ff::state
    {
    public:
        title_state();

        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual size_t child_state_count() override;
        virtual ff::state* child_state(size_t index) override;

    private:
        std::shared_ptr<retron::ui_view_state> view_state;
        Noesis::Ptr<retron::title_page> title_page;
    };
}
