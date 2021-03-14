#include "pch.h"
#include "source/core/options.h"
#include "source/main.h"
#include "source/states/app_state.h"
#include "source/ui/debug_page.xaml.h"
#include "source/ui/title_page.xaml.h"

static const std::string_view NOESIS_NAME = "d704047b-5bd2-4757-9858-6a7d86cdd006";
static const std::string_view NOESIS_KEY = "B/+52Cy2udj84658Qvc+tLTo9Yv7ZNqBhmykexFIA6nbnVl0";
static std::weak_ptr<retron::app_state> weak_app_state;

namespace
{
    namespace assets_main
    {
#include "assets.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }

    namespace assets_graphics
    {
#include "assets.graphics.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }

    namespace assets_xaml
    {
#include "assets.xaml.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }

    namespace assets_values
    {
#include "values.res.h"

        static std::shared_ptr<::ff::data_base> data()
        {
            return std::make_shared<::ff::data_static>(ff::build_res::bytes, ff::build_res::byte_size);
        }
    }
}

static void register_components()
{
    ff::resource_objects::register_global_dict(assets_main::data());
    ff::resource_objects::register_global_dict(assets_graphics::data());
    ff::resource_objects::register_global_dict(assets_xaml::data());
    ff::resource_objects::register_global_dict(assets_values::data());

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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    ff::init_app init_app(retron::get_app_params(), retron::get_ui_params());
    return ff::handle_messages_until_quit();
}

#endif
