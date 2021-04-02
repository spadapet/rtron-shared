#include "pch.h"
#include "source/core/app_service.h"
#include "source/ui/debug_page.xaml.h"

NS_IMPLEMENT_REFLECTION(retron::debug_page_view_model, "retron.debug_page_view_model")
{
    NsProp("restart_level_command", &retron::debug_page_view_model::restart_level_command);
    NsProp("restart_game_command", &retron::debug_page_view_model::restart_game_command);
    NsProp("rebuild_resources_command", &retron::debug_page_view_model::rebuild_resources_command);
    NsProp("particle_lab_command", &retron::debug_page_view_model::particle_lab_command);
    NsProp("close_debug_command", &retron::debug_page_view_model::close_debug_command);
}

retron::debug_page_view_model::debug_page_view_model()
    : restart_level_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::debug_page_view_model::debug_command, this, commands::ID_DEBUG_RESTART_LEVEL)))
    , restart_game_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::debug_page_view_model::debug_command, this, commands::ID_DEBUG_RESTART_GAME)))
    , rebuild_resources_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::debug_page_view_model::debug_command, this, commands::ID_DEBUG_REBUILD_RESOURCES)))
    , particle_lab_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::debug_page_view_model::debug_command, this, commands::ID_DEBUG_PARTICLE_LAB)))
    , close_debug_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::debug_page_view_model::debug_command, this, commands::ID_DEBUG_HIDE_UI)))
{}

void retron::debug_page_view_model::debug_command(size_t command_id)
{
    if (command_id != commands::ID_DEBUG_HIDE_UI)
    {
        retron::app_service::get().debug_command(commands::ID_DEBUG_HIDE_UI);
    }

    retron::app_service::get().debug_command(command_id);
}

NS_IMPLEMENT_REFLECTION(retron::debug_page, "retron.debug_page")
{
    NsProp("view_model", &retron::debug_page::view_model);
}

retron::debug_page::debug_page()
    : view_model_(*new debug_page_view_model())
{
    Noesis::GUI::LoadComponent(this, "debug_page.xaml");
}

retron::debug_page_view_model* retron::debug_page::view_model() const
{
    return this->view_model_;
}
