#pragma once

namespace retron
{
    class game_service;
    class level;

    class ready_state : public ff::state
    {
    public:
        ready_state(const retron::game_service& game_service, std::shared_ptr<retron::level> level, std::shared_ptr<ff::state> next_state);

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

    private:
        void init_resources();

        std::forward_list<ff::signal_connection> connections;
        const retron::game_service& game_service;
        std::shared_ptr<retron::level> level;
        std::shared_ptr<ff::state> next_state;
        ff::auto_resource<ff::sprite_font> game_font;
        size_t counter;
    };
}
