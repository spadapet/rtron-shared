#include "pch.h"
#include "source/core/app_service.h"
#include "source/states/debug_state.h"
#include "source/ui/debug_page.xaml.h"

NS_IMPLEMENT_REFLECTION(retron::debug_page_view_model, "retron.debug_page_view_model")
{
    NsProp("restart_level_command", &retron::debug_page_view_model::restart_level_command);
    NsProp("restart_game_command", &retron::debug_page_view_model::restart_game_command);
    NsProp("rebuild_resources_command", &retron::debug_page_view_model::rebuild_resources_command);
    NsProp("close_debug_command", &retron::debug_page_view_model::close_debug_command);
}

retron::debug_page_view_model::debug_page_view_model(retron::debug_state* debug_state)
    : debug_state(debug_state)
    , restart_level_command(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::debug_page_view_model::restart_level)))
    , restart_game_command(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::debug_page_view_model::restart_game)))
    , rebuild_resources_command(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::debug_page_view_model::rebuild_resources)))
    , close_debug_command(Noesis::MakePtr<ff::ui::delegate_command>(Noesis::MakeDelegate(this, &retron::debug_page_view_model::close_debug)))
{}

void retron::debug_page_view_model::restart_level(Noesis::BaseComponent* param)
{
    this->debug_state->restart_level_event_.notify();
}

void retron::debug_page_view_model::restart_game(Noesis::BaseComponent* param)
{
    this->debug_state->restart_game_event_.notify();
}

void retron::debug_page_view_model::rebuild_resources(Noesis::BaseComponent* param)
{
    this->debug_state->rebuild_resources_event_.notify();
}

void retron::debug_page_view_model::close_debug(Noesis::BaseComponent* param)
{
    this->debug_state->hide();
}

NS_IMPLEMENT_REFLECTION(retron::debug_page, "retron.debug_page")
{
    NsProp("view_model", &retron::debug_page::view_model);
}

retron::debug_page::debug_page(retron::debug_state* debug_state)
    : view_model_(*new debug_page_view_model(debug_state))
{
    Noesis::GUI::LoadComponent(this, "debug_page.xaml");
}

retron::debug_page_view_model* retron::debug_page::view_model() const
{
    return this->view_model_;
}
