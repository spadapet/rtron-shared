#pragma once

namespace retron
{
    class score_state : public ff::state
    {
    public:
        score_state(const std::vector<const retron::player*>& players, size_t active_player);

        // state
        virtual void render() override;

    private:
        void init_resources();

        std::forward_list<ff::signal_connection> connections;
        std::vector<const retron::player*> players;
        ff::auto_resource<ff::sprite_base> player_sprite;
        ff::auto_resource<ff::sprite_font> game_font;
        size_t active_player;
    };
}
