#include "pch.h"
#include "source/core/audio.h"
#include "source/states/app_state.h"
#include "source/states/debug_state.h"
#include "source/states/game_state.h"
#include "source/states/title_state.h"
#include "source/states/transition_state.h"

static const float PALETTE_CYCLES_PER_SECOND = 0.25f;
static retron::app_service* app_service;

retron::app_service& retron::app_service::get()
{
    assert(::app_service);
    return *::app_service;
}

retron::app_state::app_state()
    : viewport(ff::point_int(constants::RENDER_WIDTH, constants::RENDER_HEIGHT))
    , draw_device_(ff::draw_device::create())
    , debug_stepping_frames(false)
    , debug_step_one_frame(false)
    , debug_time_scale(1.0)
    , rebulding_resources(false)
    , render_debug_(false)
{
    assert(!::app_service);
    ::app_service = this;

    this->init_options();
    this->init_resources();
    this->init_debug_state();
    this->init_game_state();
    this->apply_system_options();
}

retron::app_state::~app_state()
{
    this->destroyed_signal.notify();

    assert(::app_service == this);
    ::app_service = nullptr;
}

std::shared_ptr<ff::state> retron::app_state::advance_time()
{
    for (auto& i : this->player_palettes)
    {
        if (i)
        {
            i->advance();
        }
    }

    return ff::state::advance_time();
}

void retron::app_state::advance_input()
{
    if (this->game_spec_.allow_debug || DEBUG)
    {
        if (this->debug_input_events->advance())
        {
            if (this->debug_input_events->event_hit(input_events::ID_DEBUG_CANCEL_STEP_ONE_FRAME))
            {
                this->debug_step_one_frame = false;
                this->debug_stepping_frames = false;
            }

            if (this->debug_input_events->event_hit(input_events::ID_DEBUG_STEP_ONE_FRAME))
            {
                this->debug_step_one_frame = this->debug_stepping_frames;
                this->debug_stepping_frames = true;
            }

            if (this->debug_input_events->event_hit(input_events::ID_DEBUG_RENDER_TOGGLE))
            {
                this->render_debug_ = !this->render_debug_;
            }
        }

        if (this->debug_input_events->digital_value(input_events::ID_DEBUG_SPEED_FAST))
        {
            this->debug_time_scale = 4.0;
        }
        else if (this->debug_input_events->digital_value(input_events::ID_DEBUG_SPEED_SLOW))
        {
            this->debug_time_scale = 0.25;
        }
        else
        {
            this->debug_time_scale = 1.0;
        }
    }
    else
    {
        this->debug_step_one_frame = false;
        this->debug_stepping_frames = false;
        this->debug_time_scale = 1.0;
    }

    ff::state::advance_input();
}

void retron::app_state::frame_rendered(ff::state::advance_t type, ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    this->debug_step_one_frame = false;

    ff::rect_fixed target_rect = this->viewport.view(target.size().rotated_pixel_size()).cast<ff::fixed_int>();
    ff::point_fixed target_scale = target_rect.size() / constants::RENDER_SIZE;
    ff::point_fixed target_pos = target_rect.top_left();

    for (ff::ui_view* view : ff::ui::rendered_views())
    {
        view->set_view_to_screen_transform(target_pos.cast<float>(), target_scale.cast<float>());
    }

    ff::state::frame_rendered(type, target, depth);
}

void retron::app_state::save_settings()
{
    this->system_options_.full_screen = ff::app_render_target().full_screen();

    ff::dict dict = ff::settings(strings::ID_APP_STATE);
    dict.set_struct(strings::ID_SYSTEM_OPTIONS, this->system_options_);
    dict.set_struct(strings::ID_GAME_OPTIONS, this->game_options_);

    ff::settings(strings::ID_APP_STATE, dict);

    ff::state::save_settings();
}

size_t retron::app_state::child_state_count()
{
    return 1;
}

ff::state* retron::app_state::child_state(size_t index)
{
    return this->debug_state && this->debug_state->visible()
        ? static_cast<ff::state*>(this->debug_state.get())
        : static_cast<ff::state*>(this->game_state.get());
}

retron::audio& retron::app_state::audio()
{
    return *this->audio_;
}

const retron::system_options& retron::app_state::system_options() const
{
    return this->system_options_;
}

const retron::game_options& retron::app_state::default_game_options() const
{
    return this->game_options_;
}

const retron::game_spec& retron::app_state::game_spec() const
{
    return this->game_spec_;
}

void retron::app_state::system_options(const retron::system_options& options)
{
    this->system_options_ = options;
    this->apply_system_options();
}

void retron::app_state::default_game_options(const retron::game_options& options)
{
    this->game_options_ = options;
}

ff::palette_base& retron::app_state::palette()
{
    return this->player_palette(0);
}

ff::palette_base& retron::app_state::player_palette(size_t player)
{
    return *this->player_palettes[player];
}

ff::draw_device& retron::app_state::draw_device() const
{
    return *this->draw_device_;
}

ff::signal_sink<void>& retron::app_state::destroyed()
{
    return this->destroyed_signal;
}

ff::signal_sink<void>& retron::app_state::reload_resources_sink()
{
    return this->reload_resources_signal;
}

bool retron::app_state::render_debug() const
{
    return this->render_debug_;
}

double retron::app_state::time_scale() const
{
    return this->debug_time_scale;
}

ff::state::advance_t retron::app_state::advance_type() const
{
    if (this->debug_step_one_frame)
    {
        return ff::state::advance_t::single_step;
    }

    if (this->debug_stepping_frames)
    {
        return ff::state::advance_t::stopped;
    }

    return ff::state::advance_t::running;
}

void retron::app_state::init_options()
{
    ff::dict dict = ff::settings(strings::ID_APP_STATE);

    if (!dict.get_struct(strings::ID_SYSTEM_OPTIONS, this->system_options_) || this->system_options_.version != retron::system_options::CURRENT_VERSION)
    {
        this->system_options_ = retron::system_options();
    }

    if (!dict.get_struct(strings::ID_GAME_OPTIONS, this->game_options_) || this->game_options_.version != retron::game_options::CURRENT_VERSION)
    {
        this->game_options_ = retron::game_options();
    }
}

// Must be able to be called multiple times (whenever resources are hot reloaded)
void retron::app_state::init_resources()
{
    this->audio_ = std::make_unique<retron::audio>();
    this->game_spec_ = retron::game_spec::load();
    this->debug_input_mapping = "game_debug_controls"sv;
    this->debug_input_events = std::make_unique<ff::input_event_provider>(*this->debug_input_mapping.object(), std::vector<const ff::input_vk*>{ &ff::input::keyboard() });
    this->palette_data = "palette_main"sv;

    for (size_t i = 0; i < this->player_palettes.size(); i++)
    {
        std::ostringstream str;
        str << "player_" << i;
        this->player_palettes[i] = std::make_shared<ff::palette_cycle>(this->palette_data.object(), str.str().c_str(), ::PALETTE_CYCLES_PER_SECOND);
    }
}

void retron::app_state::init_debug_state()
{
    this->debug_state = std::make_shared<retron::debug_state>();

    this->connections.emplace_front(ff::custom_debug_sink().connect(std::bind(&retron::app_state::on_custom_debug, this)));
    this->connections.emplace_front(ff::resource_objects::rebuilt_global_sink().connect(std::bind(&retron::app_state::on_resources_rebuilt, this)));
    this->connections.emplace_front(this->debug_state->restart_level_event().connect(std::bind(&retron::app_state::on_restart_level, this)));
    this->connections.emplace_front(this->debug_state->restart_game_event().connect(std::bind(&retron::app_state::on_restart_game, this)));
    this->connections.emplace_front(this->debug_state->rebuild_resources_event().connect(std::bind(&retron::app_state::on_rebuild_resources, this)));
}

void retron::app_state::on_custom_debug()
{
    if (this->debug_state->visible())
    {
        this->debug_state->hide();
    }
    else if (this->game_spec_.allow_debug || DEBUG)
    {
        this->debug_state->visible(this->game_state);
    }
}

void retron::app_state::on_resources_rebuilt()
{
    this->rebulding_resources = false;
    this->init_resources();
    this->reload_resources_signal.notify();
}

void retron::app_state::on_restart_level()
{
    this->debug_state->hide();

    std::shared_ptr<retron::game_state> game_state = std::dynamic_pointer_cast<retron::game_state>(this->game_state->wrapped_state());
    if (game_state)
    {
        game_state->restart_level();
    }
    else
    {
        this->init_game_state();
    }
}

void retron::app_state::on_restart_game()
{
    this->debug_state->hide();
    this->init_game_state();
}

void retron::app_state::on_rebuild_resources()
{
    this->debug_state->hide();

    if (!this->rebulding_resources)
    {
        this->rebulding_resources = true;
        ff::resource_objects::rebuild_global_async();
    }
}

void retron::app_state::init_game_state()
{
    std::shared_ptr<ff::state> title_state = std::make_shared<retron::title_state>();
    std::shared_ptr<retron::transition_state> transition_state = std::make_shared<retron::transition_state>(nullptr, title_state, "transition_bg_1.png", 4, 24);

    this->game_state = std::make_shared<ff::state_wrapper>(transition_state);
}

void retron::app_state::apply_system_options()
{
    ff::app_render_target().full_screen(this->system_options_.full_screen);
}
