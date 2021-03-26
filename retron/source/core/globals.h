#pragma once

namespace strings
{
    extern const std::string_view ID_APP_STATE;
    extern const std::string_view ID_GAME_OPTIONS;
    extern const std::string_view ID_SYSTEM_OPTIONS;
}

namespace colors
{
    const int LEVEL_BORDER = 77;
}

namespace input_events
{
    extern const size_t ID_UP;
    extern const size_t ID_DOWN;
    extern const size_t ID_LEFT;
    extern const size_t ID_RIGHT;
    extern const size_t ID_SHOOT_UP;
    extern const size_t ID_SHOOT_DOWN;
    extern const size_t ID_SHOOT_LEFT;
    extern const size_t ID_SHOOT_RIGHT;
    extern const size_t ID_ACTION;
    extern const size_t ID_CANCEL;
    extern const size_t ID_PAUSE;
    extern const size_t ID_START;

    extern const size_t ID_DEBUG_STEP_ONE_FRAME;
    extern const size_t ID_DEBUG_CANCEL_STEP_ONE_FRAME;
    extern const size_t ID_DEBUG_SPEED_SLOW;
    extern const size_t ID_DEBUG_SPEED_FAST;
    extern const size_t ID_DEBUG_RENDER_TOGGLE;
}

namespace constants
{
    const size_t MAX_PLAYERS = 2;
    const ff::fixed_int LEVEL_BORDER_THICKNESS = 2;
    const ff::fixed_int LEVEL_BOX_THICKNESS = 2;

    const ff::fixed_int RENDER_WIDTH = 480; // 1920 / 4
    const ff::fixed_int RENDER_HEIGHT = 270; // 1080 / 4
    const ff::point_fixed RENDER_SIZE(RENDER_WIDTH, RENDER_HEIGHT);
    const ff::rect_fixed RENDER_RECT(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

    const ff::fixed_int RENDER_SCALE = 4; // scale to a 1080p buffer, which then gets scaled to the screen

    const ff::fixed_int RENDER_WIDTH_HIGH = 1920;
    const ff::fixed_int RENDER_HEIGHT_HIGH = 1080;
    const ff::point_fixed RENDER_SIZE_HIGH(RENDER_WIDTH_HIGH, RENDER_HEIGHT_HIGH);
    const ff::rect_fixed RENDER_RECT_HIGH(0, 0, RENDER_WIDTH_HIGH, RENDER_HEIGHT_HIGH);
}

namespace helpers
{
    ff::fixed_int dir_to_degrees(ff::point_fixed dir);
    size_t dir_to_index(ff::point_fixed dir); // degrees = index * 45
}
