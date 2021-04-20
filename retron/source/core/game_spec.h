#pragma once

namespace retron
{
    struct level_rect
    {
        enum class type
        {
            none,
            bounds,
            box,
            safe,
            objects,
        };

        type type;
        ff::rect_fixed rect;
    };

    struct level_objects_spec : public retron::level_rect
    {
        size_t electrode;
        size_t grunt;
        size_t hulk;
        size_t bonus_woman;
        size_t bonus_man;
        size_t bonus_child;
    };

    struct level_spec
    {
        std::vector<retron::level_rect> rects;
        std::vector<retron::level_objects_spec> objects;
        ff::point_fixed player_start;
    };

    struct level_set_spec
    {
        std::vector<std::string> levels;
        size_t loop_start;
    };

    struct difficulty_spec
    {
        std::string name;
        std::string level_set;
        size_t lives;
        size_t grunt_tick_frames; // grunts only move each "tick" number of frames
        size_t grunt_max_ticks; // random 1-max ticks per move
        size_t grunt_max_ticks_rate; // frames to decrease max ticks
        size_t grunt_min_ticks;
        size_t player_shot_counter;
        ff::fixed_int grunt_move;
        ff::fixed_int player_move;
        ff::fixed_int player_move_frame_divisor;
        ff::fixed_int player_shot_move;
        ff::fixed_int player_shot_start_offset;
    };

    struct game_spec
    {
        static retron::game_spec load();

        bool allow_debug() const;

        bool allow_debug_;
        ff::fixed_int joystick_min;
        ff::fixed_int joystick_max;
        std::unordered_map<std::string, retron::difficulty_spec> difficulties;
        std::unordered_map<std::string, retron::level_set_spec> level_sets;
        std::unordered_map<std::string, retron::level_spec> levels;
    };

    struct player
    {
        retron::player* coop;
        size_t index;
        size_t level;
        size_t lives;
        size_t score;
    };
}
