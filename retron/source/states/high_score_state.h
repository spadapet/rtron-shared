#pragma once

namespace retron
{
    class high_score_state : public ff::state
    {
    public:
        high_score_state();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;

    private:
    };
}
