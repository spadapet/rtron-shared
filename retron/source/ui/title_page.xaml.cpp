#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/options.h"
#include "source/game/game_state.h"
#include "source/ui/title_page.xaml.h"

NS_IMPLEMENT_REFLECTION(retron::title_page_view_model, "retron.title_page_view_model")
{
    NsProp("players_text", &retron::title_page_view_model::players_text);
    NsProp("difficulty_text", &retron::title_page_view_model::difficulty_text);
    NsProp("sound_text", &retron::title_page_view_model::sound_text);
    NsProp("full_screen_text", &retron::title_page_view_model::full_screen_text);

    NsProp("start_game_command", &retron::title_page_view_model::start_game_command_);
    NsProp("players_command", &retron::title_page_view_model::players_command_);
    NsProp("difficulty_command", &retron::title_page_view_model::difficulty_command_);
    NsProp("sound_command", &retron::title_page_view_model::sound_command_);
    NsProp("full_screen_command", &retron::title_page_view_model::full_screen_command_);
    NsProp("state_back_command", &retron::title_page_view_model::state_back_command_);
}

retron::title_page_view_model::title_page_view_model()
    : visual_state_root_(nullptr)
    , start_game_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::start_game_command)))
    , players_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::players_command)))
    , difficulty_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::difficulty_command)))
    , sound_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::sound_command)))
    , full_screen_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::full_screen_command)))
    , state_back_command_(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::title_page_view_model::state_back_command)))
{
    this->target_size_changed_connection = ff::app_render_target().size_changed().connect([this](ff::window_size size)
        {
            this->property_changed("full_screen_text");
        });
}

void retron::title_page_view_model::visual_state_root(Noesis::FrameworkElement* value)
{
    this->visual_state_root_ = value;
}

std::shared_ptr<ff::state> retron::title_page_view_model::pending_state() const
{
    return this->pending_state_;
}

const char* retron::title_page_view_model::players_text() const
{
    switch (retron::app_service::get().default_game_options().players)
    {
        default: return "";
        case retron::game_players::one: return "One";
        case retron::game_players::two_take_turns: return "Two (Swap)";
        case retron::game_players::two_coop: return "Two (Co-Op)";
    }
}

const char* retron::title_page_view_model::difficulty_text() const
{
    switch (retron::app_service::get().default_game_options().difficulty)
    {
        default: return "";
        case retron::game_difficulty::baby: return "Baby";
        case retron::game_difficulty::easy: return "Easy";
        case retron::game_difficulty::normal: return "Normal";
        case retron::game_difficulty::hard: return "Hard";
    }
}

const char* retron::title_page_view_model::sound_text() const
{
    return retron::app_service::get().system_options().sound ? "On" : "Off";
}

const char* retron::title_page_view_model::full_screen_text() const
{
    return ff::app_render_target().full_screen() ? "On" : "Off";
}

void retron::title_page_view_model::start_game_command(Noesis::BaseComponent* param)
{
    this->pending_state_ = std::make_shared<retron::game_state>();
}

void retron::title_page_view_model::players_command(Noesis::BaseComponent* param)
{
    bool forward = !Noesis::Boxing::CanUnbox<bool>(param) || Noesis::Boxing::Unbox<bool>(param);
    retron::game_options options = retron::app_service::get().default_game_options();

    if (forward)
    {
        options.players = (options.players == retron::game_players::two_coop)
            ? retron::game_players::one
            : static_cast<retron::game_players>(static_cast<int>(options.players) + 1);
    }
    else
    {
        options.players = (options.players == retron::game_players::one)
            ? retron::game_players::two_coop
            : static_cast<retron::game_players>(static_cast<int>(options.players) - 1);
    }

    retron::app_service::get().default_game_options(options);
    this->property_changed("players_text");
}

void retron::title_page_view_model::difficulty_command(Noesis::BaseComponent* param)
{
    bool forward = !Noesis::Boxing::CanUnbox<bool>(param) || Noesis::Boxing::Unbox<bool>(param);
    retron::game_options options = retron::app_service::get().default_game_options();

    if (forward)
    {
        options.difficulty = (options.difficulty == retron::game_difficulty::hard)
            ? retron::game_difficulty::easy
            : static_cast<retron::game_difficulty>(static_cast<int>(options.difficulty) + 1);
    }
    else
    {
        options.difficulty = (options.difficulty == retron::game_difficulty::easy)
            ? retron::game_difficulty::hard
            : static_cast<retron::game_difficulty>(static_cast<int>(options.difficulty) - 1);
    }

    retron::app_service::get().default_game_options(options);
    this->property_changed("difficulty_text");
}

void retron::title_page_view_model::sound_command(Noesis::BaseComponent* param)
{
    retron::system_options options = retron::app_service::get().system_options();
    options.sound = !options.sound;
    retron::app_service::get().system_options(options);
    this->property_changed("sound_text");
}

void retron::title_page_view_model::full_screen_command(Noesis::BaseComponent* param)
{
    ff::app_render_target().full_screen(!ff::app_render_target().full_screen());
}

void retron::title_page_view_model::state_back_command(Noesis::BaseComponent* param)
{
    Noesis::VisualStateGroupCollection* groups = Noesis::VisualStateManager::GetVisualStateGroups(this->visual_state_root_);
    assert(groups);

    for (unsigned int i = 0; i < static_cast<unsigned int>(groups->Count()); i++)
    {
        Noesis::VisualStateGroup* group = groups->Get(i);
        if (group->GetName() && !std::strcmp(group->GetName(), "main_group"))
        {
            Noesis::VisualState* state = group->GetCurrentState(this->visual_state_root_);
            if (state && !std::strcmp(state->GetName().Str(), "initial_state"))
            {
                ff::window::main()->close();
            }
            else
            {
                Noesis::VisualStateManager::GoToElementState(this->visual_state_root_, Noesis::Symbol("initial_state"), true);
            }

            break;
        }
    }
}

NS_IMPLEMENT_REFLECTION(retron::title_page, "retron.title_page")
{
    NsProp("view_model", &retron::title_page::view_model);
}

retron::title_page::title_page()
    : view_model_(*new title_page_view_model())
{
    Noesis::GUI::LoadComponent(this, "title_page.xaml");
    this->view_model_->visual_state_root(FindName<Noesis::FrameworkElement>("root_panel"));
}

retron::title_page_view_model* retron::title_page::view_model() const
{
    return this->view_model_;
}
