#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/options.h"
#include "source/main.h"
#include "source/states/app_state.h"
#include "source/ui/debug_page.xaml.h"
#include "source/ui/title_page.xaml.h"

static const std::string_view NOESIS_NAME = "d704047b-5bd2-4757-9858-6a7d86cdd006";
static const std::string_view NOESIS_KEY = "B/+52Cy2udj84658Qvc+tLTo9Yv7ZNqBhmykexFIA6nbnVl0";
static std::weak_ptr<retron::app_state> weak_app_state;

namespace assets
{
    namespace controls
    {
#include "controls.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }


    namespace game_spec
    {
#include "game_spec.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }

    namespace graphics
    {
#include "graphics.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }

    namespace xaml
    {
#include "xaml.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }
}

static void register_components()
{
    ff::resource_objects::register_global_dict(assets::controls::data());
    ff::resource_objects::register_global_dict(assets::game_spec::data());
    ff::resource_objects::register_global_dict(assets::graphics::data());
    ff::resource_objects::register_global_dict(assets::xaml::data());

    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_flags>>();
    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_players>>();
    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_difficulty>>();

    Noesis::RegisterComponent(Noesis::TypeOf<retron::debug_page>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::debug_page_view_model>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::title_page>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::title_page_view_model>(), nullptr);
}

static std::shared_ptr<ff::state> create_app_state()
{
    assert(::weak_app_state.expired());

    auto app_state = std::make_shared<retron::app_state>();
    ::weak_app_state = app_state;
    return app_state;
}

static const ff::palette_base* get_ui_palette()
{
    auto app_state = ::weak_app_state.lock();
    return app_state ? &app_state->palette() : nullptr;
}

static double get_time_scale()
{
    auto app_state = ::weak_app_state.lock();
    return app_state ? app_state->time_scale() : 1;
}

static ff::state::advance_t get_advance_type()
{
    auto app_state = ::weak_app_state.lock();
    return app_state ? app_state->advance_type() : ff::state::advance_t::running;
}

ff::init_app_params retron::get_app_params()
{
    ff::init_app_params params{};
    params.create_initial_state_func = ::create_app_state;
    params.get_time_scale_func = ::get_time_scale;
    params.get_advance_type_func = ::get_advance_type;

    return params;
}

ff::init_ui_params retron::get_ui_params()
{
    ff::init_ui_params params{};
    params.application_resources_name = "application_resources.xaml";
    params.default_font = "fonts/#Robotron 2084";
    params.default_font_size = 8;
    params.noesis_license_name = ::NOESIS_NAME;
    params.noesis_license_key = ::NOESIS_KEY;
    params.palette_func = ::get_ui_palette;
    params.register_components_func = ::register_components;

    return params;
}

#if !UWP_APP

static void set_window_client_size(HWND hwnd, const ff::point_int& new_size)
{
    RECT rect{ 0, 0, new_size.x, new_size.y }, rect2;
    const DWORD style = static_cast<DWORD>(GetWindowLong(hwnd, GWL_STYLE));
    const DWORD exStyle = static_cast<DWORD>(GetWindowLong(hwnd, GWL_EXSTYLE));

    HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);

    if (monitor &&
        ::AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, ::GetDpiForWindow(hwnd)) &&
        ::GetMonitorInfo(monitor, &mi) &&
        ::GetWindowRect(hwnd, &rect2))
    {
        ff::rect_int monitor_rect(mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);
        ff::rect_int window_rect(rect2.left, rect2.top, rect2.left + rect.right - rect.left, rect2.top + rect.bottom - rect.top);

        window_rect = window_rect.move_inside(monitor_rect).crop(monitor_rect);

        ::SetWindowPos(hwnd, nullptr, window_rect.left, window_rect.top, window_rect.width(), window_rect.height(),
            SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }
}

static void handle_window_message(ff::window_message& msg)
{
    switch (msg.msg)
    {
        case WM_SYSKEYDOWN:
            if ((msg.wp == VK_DELETE || msg.wp == VK_PRIOR || msg.wp == VK_NEXT) &&
                retron::app_service::get().game_spec().allow_debug &&
                ::GetKeyState(VK_SHIFT) < 0 &&
                !ff::app_render_target().full_screen())
            {
                // Shift-Alt-Del: Force 1080p client area
                // Shift-Alt-PgUp|PgDown: Force multiples of 1080p client area

                RECT rect;
                if (::GetClientRect(msg.hwnd, &rect))
                {
                    const std::array<ff::point_int, 4> sizes
                    {
                        ff::point_int(480, 270),
                        ff::point_int(960, 540),
                        ff::point_int(1920, 1080),
                        ff::point_int(3840, 2160),
                    };

                    const ff::point_int old_size(rect.right, rect.bottom);
                    ff::point_int new_size = (msg.wp == VK_DELETE) ? sizes[2] : old_size;

                    if (msg.wp == VK_PRIOR)
                    {
                        for (auto i = sizes.crbegin(); i != sizes.crend(); i++)
                        {
                            if (i->x < new_size.x)
                            {
                                new_size = *i;
                                break;
                            }
                        }
                    }
                    else if (msg.wp == VK_NEXT)
                    {
                        for (auto i = sizes.cbegin(); i != sizes.cend(); i++)
                        {
                            if (i->x > new_size.x)
                            {
                                new_size = *i;
                                break;
                            }
                        }
                    }

                    if (new_size != old_size)
                    {
                        ::set_window_client_size(msg.hwnd, new_size);
                    }
                }
            }
            break;
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    ff::init_app init_app(retron::get_app_params(), retron::get_ui_params());
    ff::signal_connection message_connection = ff::window::main()->message_sink().connect(::handle_window_message);

    return ff::handle_messages_until_quit();
}

#endif
