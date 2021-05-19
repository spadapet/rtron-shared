#pragma once

namespace retron
{
    class game_over_state : public ff::state
    {
    public:
        game_over_state(std::shared_ptr<ff::state> under_state);

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

    private:
        std::shared_ptr<ff::state> under_state;
        ff::auto_resource<ff::sprite_font> game_font;
    };
}
