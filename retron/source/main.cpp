#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/options.h"
#include "source/states/app_state.h"
#include "source/ui/debug_page.xaml.h"
#include "source/ui/particle_lab_page.xaml.h"
#include "source/ui/title_page.xaml.h"

namespace res
{
    void register_bonus();
    void register_controls();
    void register_electrode();
    void register_game_spec();
    void register_graphics();
    void register_particles();
    void register_player();
    void register_sprites();
    void register_xaml();
}

static const std::string_view NOESIS_NAME = "d704047b-5bd2-4757-9858-6a7d86cdd006";
static const std::string_view NOESIS_KEY = "c02GygIEhsoRJpvqrPLSUToofFKSJ+imAbUl5jO5fcHl54P6";
static std::weak_ptr<retron::app_state> weak_app_state;

static void register_components()
{
    ::res::register_bonus();
    ::res::register_controls();
    ::res::register_electrode();
    ::res::register_game_spec();
    ::res::register_graphics();
    ::res::register_particles();
    ::res::register_player();
    ::res::register_sprites();
    ::res::register_xaml();

    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_flags>>();
    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_players>>();
    Noesis::RegisterComponent<Noesis::EnumConverter<retron::game_difficulty>>();

    Noesis::RegisterComponent(Noesis::TypeOf<retron::debug_page>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::debug_page_view_model>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::particle_lab_page>(), nullptr);
    Noesis::RegisterComponent(Noesis::TypeOf<retron::particle_lab_page_view_model>(), nullptr);
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

static ff::init_app_params get_app_params()
{
    ff::init_app_params params{};
    params.create_initial_state_func = ::create_app_state;
    params.get_time_scale_func = ::get_time_scale;
    params.get_advance_type_func = ::get_advance_type;

    return params;
}

static ff::init_ui_params get_ui_params()
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

#if UWP_APP

namespace retron
{
    ref class app : Windows::ApplicationModel::Core::IFrameworkViewSource, Windows::ApplicationModel::Core::IFrameworkView
    {
    public:
        virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
        {
            return this;
        }

        virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ view)
        {
            view->Activated += ref new Windows::Foundation::TypedEventHandler<
                Windows::ApplicationModel::Core::CoreApplicationView^,
                Windows::ApplicationModel::Activation::IActivatedEventArgs^>(
                this, &app::activated);
        }

        virtual void Load(Platform::String^ entry_point)
        {}

        virtual void Run()
        {
            ff::handle_messages_until_quit();
        }

        virtual void SetWindow(Windows::UI::Core::CoreWindow^ window)
        {}

        virtual void Uninitialize()
        {}

    private:
        void activated(Windows::ApplicationModel::Core::CoreApplicationView^ view, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
        {
            if (!this->init_app)
            {
                auto app_view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
                app_view->SetPreferredMinSize(Windows::Foundation::Size(retron::constants::RENDER_WIDTH, retron::constants::RENDER_HEIGHT));

                this->init_app = std::make_unique<ff::init_app>(::get_app_params(), ::get_ui_params());
                assert(*this->init_app);
            }

            Windows::UI::Core::CoreWindow::GetForCurrentThread()->Activate();
        }

        std::unique_ptr<ff::init_app> init_app;
    };
}

int main(Platform::Array<Platform::String^>^ args)
{
    Windows::ApplicationModel::Core::CoreApplication::Run(ref new retron::app());
    return 0;
}

#else

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
        case WM_GETMINMAXINFO:
            {
                RECT rect{ 0, 0, static_cast<int>(retron::constants::RENDER_WIDTH), static_cast<int>(retron::constants::RENDER_HEIGHT) };
                const DWORD style = static_cast<DWORD>(GetWindowLong(msg.hwnd, GWL_STYLE));
                const DWORD exStyle = static_cast<DWORD>(GetWindowLong(msg.hwnd, GWL_EXSTYLE));
                if (::AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, ::GetDpiForWindow(msg.hwnd)))
                {
                    MINMAXINFO& mm = *reinterpret_cast<MINMAXINFO*>(msg.lp);
                    mm.ptMinTrackSize.x = rect.right - rect.left;
                    mm.ptMinTrackSize.y = rect.bottom - rect.top;
                }
            }
            break;

        case WM_SYSKEYDOWN:
            if ((msg.wp == VK_DELETE || msg.wp == VK_PRIOR || msg.wp == VK_NEXT) &&
                retron::app_service::get().game_spec().allow_debug() &&
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
    ff::init_app init_app(::get_app_params(), ::get_ui_params());
    ff::signal_connection message_connection = ff::window::main()->message_sink().connect(::handle_window_message);
    return ff::handle_messages_until_quit();
}

#endif
