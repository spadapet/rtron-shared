#pragma once

namespace retron
{
    enum class bonus_type
    {
        none,
        woman,
        man,
        boy,
        girl,
        dog,

        count
    };

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
        size_t electrode_type;
        size_t grunt;
        size_t hulk;
        size_t bonus;
        retron::bonus_type bonus_type;
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
        size_t first_free_life;
        size_t next_free_life;
        std::vector<size_t> bonus_points;

        size_t grunt_tick_frames; // grunts only move each "tick" number of frames
        size_t grunt_max_ticks; // random 1-max ticks per move
        size_t grunt_min_ticks;
        size_t grunt_max_ticks_rate; // frames to decrease max ticks
        ff::point_fixed grunt_move;

        size_t hulk_tick_frames;
        size_t hulk_max_ticks;
        size_t hulk_min_ticks;
        size_t hulk_no_move_chance;
        ff::point_fixed hulk_move;
        ff::point_fixed hulk_push;
        ff::point_fixed hulk_fudge;

        ff::fixed_int player_move;
        ff::fixed_int player_move_frame_divisor;
        ff::fixed_int player_shot_move;
        ff::fixed_int player_shot_start_offset;
        size_t player_shot_counter;
        size_t player_dead_counter;
        size_t player_ghost_counter;
        size_t player_ghost_warning_counter;
        size_t player_winning_counter;

        size_t bonus_tick_frames;
        size_t bonus_max_ticks;
        size_t bonus_min_ticks;
        ff::point_fixed bonus_move;

        size_t points_electrode;
        size_t points_grunt;
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
        player& self_or_coop();
        const player& self_or_coop() const;

        retron::player* coop;
        size_t index;
        size_t level;
        size_t lives;
        size_t points;
        size_t next_life_points;
        bool game_over;
    };
}
