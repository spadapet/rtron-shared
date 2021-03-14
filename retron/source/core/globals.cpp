#include "pch.h"
#include "source/core/globals.h"

const std::string_view strings::ID_APP_STATE = "app_state"sv;
const std::string_view strings::ID_GAME_OPTIONS = "game_options"sv;
const std::string_view strings::ID_SYSTEM_OPTIONS = "system_options"sv;

const size_t input_events::ID_UP = ff::stable_hash_func("up"sv);
const size_t input_events::ID_DOWN = ff::stable_hash_func("down"sv);
const size_t input_events::ID_LEFT = ff::stable_hash_func("left"sv);
const size_t input_events::ID_RIGHT = ff::stable_hash_func("right"sv);
const size_t input_events::ID_SHOOT_UP = ff::stable_hash_func("shoot_up"sv);
const size_t input_events::ID_SHOOT_DOWN = ff::stable_hash_func("shoot_down"sv);
const size_t input_events::ID_SHOOT_LEFT = ff::stable_hash_func("shoot_left"sv);
const size_t input_events::ID_SHOOT_RIGHT = ff::stable_hash_func("shoot_right"sv);
const size_t input_events::ID_ACTION = ff::stable_hash_func("action"sv);
const size_t input_events::ID_CANCEL = ff::stable_hash_func("cancel"sv);
const size_t input_events::ID_PAUSE = ff::stable_hash_func("pause"sv);
const size_t input_events::ID_START = ff::stable_hash_func("start"sv);

const size_t input_events::ID_DEBUG_STEP_ONE_FRAME = ff::stable_hash_func("step_one_frame"sv);
const size_t input_events::ID_DEBUG_CANCEL_STEP_ONE_FRAME = ff::stable_hash_func("cancel_step_one_frame"sv);
const size_t input_events::ID_DEBUG_SPEED_SLOW = ff::stable_hash_func("speed_slow"sv);
const size_t input_events::ID_DEBUG_SPEED_FAST = ff::stable_hash_func("speed_fast"sv);
const size_t input_events::ID_DEBUG_RENDER_TOGGLE = ff::stable_hash_func("debug_render_toggle"sv);

ff::fixed_int helpers::dir_to_degrees(ff::point_fixed dir)
{
    return ff::fixed_int(ff::math::radians_to_degrees(std::atan2f(-dir.y, dir.x)));
}
