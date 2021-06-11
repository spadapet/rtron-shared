#pragma once

namespace retron
{
    enum class level_phase
    {
        ready,
        playing,
        won,
        dead,
        game_over,
    };

    class level_base
    {
    public:
        virtual retron::level_phase phase() const = 0;
        virtual void start() = 0; // move from ready->playing
        virtual void restart() = 0; // move from dead->ready
        virtual void stop() = 0; // move from dead->game_over
        virtual const std::vector<const retron::player*>& players() const = 0;
    };
}
