#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/core/globals.h"

const std::string_view retron::strings::ID_APP_STATE = "app_state";
const std::string_view retron::strings::ID_GAME_OPTIONS = "game_options";
const std::string_view retron::strings::ID_SYSTEM_OPTIONS = "system_options";

const size_t retron::input_events::ID_UP = ff::stable_hash_func("up"sv);
const size_t retron::input_events::ID_DOWN = ff::stable_hash_func("down"sv);
const size_t retron::input_events::ID_LEFT = ff::stable_hash_func("left"sv);
const size_t retron::input_events::ID_RIGHT = ff::stable_hash_func("right"sv);
const size_t retron::input_events::ID_SHOOT_UP = ff::stable_hash_func("shoot_up"sv);
const size_t retron::input_events::ID_SHOOT_DOWN = ff::stable_hash_func("shoot_down"sv);
const size_t retron::input_events::ID_SHOOT_LEFT = ff::stable_hash_func("shoot_left"sv);
const size_t retron::input_events::ID_SHOOT_RIGHT = ff::stable_hash_func("shoot_right"sv);
const size_t retron::input_events::ID_ACTION = ff::stable_hash_func("action"sv);
const size_t retron::input_events::ID_CANCEL = ff::stable_hash_func("cancel"sv);
const size_t retron::input_events::ID_PAUSE = ff::stable_hash_func("pause"sv);
const size_t retron::input_events::ID_START = ff::stable_hash_func("start"sv);

const size_t retron::input_events::ID_DEBUG_STEP_ONE_FRAME = ff::stable_hash_func("step_one_frame"sv);
const size_t retron::input_events::ID_DEBUG_CANCEL_STEP_ONE_FRAME = ff::stable_hash_func("cancel_step_one_frame"sv);
const size_t retron::input_events::ID_DEBUG_SPEED_SLOW = ff::stable_hash_func("speed_slow"sv);
const size_t retron::input_events::ID_DEBUG_SPEED_FAST = ff::stable_hash_func("speed_fast"sv);
const size_t retron::input_events::ID_DEBUG_RENDER_TOGGLE = ff::stable_hash_func("debug_render_toggle"sv);
const size_t retron::input_events::ID_DEBUG_INVINCIBLE_TOGGLE = ff::stable_hash_func("invincible_toggle"sv);
const size_t retron::input_events::ID_DEBUG_COMPLETE_LEVEL = ff::stable_hash_func("complete_level"sv);
const size_t retron::input_events::ID_SHOW_CUSTOM_DEBUG = ff::stable_hash_func("show_custom_debug"sv);

const size_t retron::commands::ID_DEBUG_HIDE_UI = ff::stable_hash_func("debug_hide_ui"sv);
const size_t retron::commands::ID_DEBUG_PARTICLE_LAB = ff::stable_hash_func("debug_particle_lab"sv);
const size_t retron::commands::ID_DEBUG_RESTART_GAME = ff::stable_hash_func("debug_restart_game"sv);
const size_t retron::commands::ID_DEBUG_RESTART_LEVEL = ff::stable_hash_func("debug_restart_level"sv);
const size_t retron::commands::ID_DEBUG_REBUILD_RESOURCES = ff::stable_hash_func("debug_rebuild_resources"sv);

ff::fixed_int retron::helpers::dir_to_degrees(ff::point_fixed dir)
{
    ff::fixed_int angle = dir ? ff::math::radians_to_degrees(std::atan2f(-dir.y, dir.x)) : 270.0f;
    return (angle < 0_f) ? angle + 360_f : angle;
}

// degrees = index * 45
size_t retron::helpers::dir_to_index(ff::point_fixed dir)
{
    if (dir.x > 0_f)
    {
        if (dir.y < 0_f)
        {
            return 1;
        }
        else if (dir.y > 0_f)
        {
            return 7;
        }

        return 0;
    }
    else if (dir.x < 0_f)
    {
        if (dir.y < 0_f)
        {
            return 3;
        }
        else if (dir.y > 0_f)
        {
            return 5;
        }

        return 4;
    }
    else if (dir.y < 0_f)
    {
        return 2;
    }

    return 6;
}

ff::point_fixed retron::helpers::index_to_dir(size_t index)
{
    static const std::array<ff::point_fixed, 8> dirs
    {
        ff::point_fixed(1, 0), // 0
        ff::point_fixed(1, 1), // 1
        ff::point_fixed(0, 1), // 2
        ff::point_fixed(-1, 1), // 3
        ff::point_fixed(-1, 0), // 4
        ff::point_fixed(-1, -1), // 5
        ff::point_fixed(0, -1), // 6
        ff::point_fixed(1, -1), // 7
    };

    return dirs[index % dirs.size()];
}

ff::point_fixed retron::helpers::canon_dir(const ff::point_fixed& value)
{
    return ff::point_fixed(
        std::copysign(1_f, value.x) * ff::fixed_int(value.x != 0_f),
        std::copysign(1_f, value.y) * ff::fixed_int(value.y != 0_f));
}

ff::point_fixed retron::helpers::get_press_vector(const ff::input_event_provider& input_events, bool for_shoot)
{
    ff::fixed_int joystick_min = retron::app_service::get().game_spec().joystick_min;

    ff::rect_fixed dir_press(
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_LEFT : retron::input_events::ID_LEFT),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_UP : retron::input_events::ID_UP),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_RIGHT : retron::input_events::ID_RIGHT),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_DOWN : retron::input_events::ID_DOWN));

    dir_press.left = dir_press.left * ff::fixed_int(dir_press.left >= joystick_min);
    dir_press.top = dir_press.top * ff::fixed_int(dir_press.top >= joystick_min);
    dir_press.right = dir_press.right * ff::fixed_int(dir_press.right >= joystick_min);
    dir_press.bottom = dir_press.bottom * ff::fixed_int(dir_press.bottom >= joystick_min);

    ff::point_fixed dir(dir_press.right - dir_press.left, dir_press.bottom - dir_press.top);
    if (dir)
    {
        int slice = retron::helpers::dir_to_degrees(dir) * 2 / 45;

        return ff::point_fixed(
            (slice >= 6 && slice <= 11) ? -1 : ((slice <= 3 || slice >= 14) ? 1 : 0),
            (slice >= 2 && slice <= 7) ? -1 : ((slice >= 10 && slice <= 15) ? 1 : 0));
    }

    return ff::point_fixed(0, 0);
}
