#include "pch.h"
#include "source/core/app_service.h"
#include "source/ui/particle_lab_page.xaml.h"

NS_IMPLEMENT_REFLECTION(retron::particle_lab_page_view_model, "retron.particle_lab_page_view_model")
{
    NsProp("close_debug_command", &retron::particle_lab_page_view_model::close_debug_command);
    NsProp("rebuild_resources_command", &retron::particle_lab_page_view_model::rebuild_resources_command);
    NsProp("particle_effects", &retron::particle_lab_page_view_model::particle_effects);
    NsProp("selected_particle_effect", &retron::particle_lab_page_view_model::selected_particle_effect, &retron::particle_lab_page_view_model::selected_particle_effect);
}

retron::particle_lab_page_view_model::particle_lab_page_view_model()
    : close_debug_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::particle_lab_page_view_model::debug_command, this, commands::ID_DEBUG_HIDE_UI)))
    , rebuild_resources_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::particle_lab_page_view_model::debug_command, this, commands::ID_DEBUG_REBUILD_RESOURCES)))
    , particle_effects(*new Noesis::ObservableCollection<Noesis::BaseComponent>())
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::particle_lab_page_view_model::init_particle_effects, this)));
    this->init_particle_effects();
}

Noesis::BaseComponent* retron::particle_lab_page_view_model::selected_particle_effect() const
{
    return this->selected_particle_effect_;
}

void retron::particle_lab_page_view_model::selected_particle_effect(Noesis::BaseComponent* value)
{
    if (value && selected_particle_effect_ != value)
    {
        this->selected_particle_effect_ = *value;
        this->property_changed("selected_particle_effect");
    }
}

void retron::particle_lab_page_view_model::init_particle_effects()
{
    this->particle_effects->Clear();
    this->particle_effects->Add(Noesis::Boxing::Box("Foo 1"));
    this->particle_effects->Add(Noesis::Boxing::Box("Bar 1"));
    this->particle_effects->Add(Noesis::Boxing::Box("Bar 2"));

    this->selected_particle_effect_ = *this->particle_effects->Get(0);
}

void retron::particle_lab_page_view_model::debug_command(size_t command_id)
{
    retron::app_service::get().debug_command(command_id);
}

NS_IMPLEMENT_REFLECTION(retron::particle_lab_page, "retron.particle_lab_page")
{
    NsProp("view_model", &retron::particle_lab_page::view_model);
}

retron::particle_lab_page::particle_lab_page()
    : view_model_(*new particle_lab_page_view_model())
{
    Noesis::GUI::LoadComponent(this, "particle_lab_page.xaml");
}

retron::particle_lab_page_view_model* retron::particle_lab_page::view_model() const
{
    return this->view_model_;
}
