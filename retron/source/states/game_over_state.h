#pragma once

namespace retron
{
    class game_over_state : public ff::state
    {
    public:
        game_over_state();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;

    private:
    };
}
