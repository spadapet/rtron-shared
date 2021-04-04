#include "pch.h"
#include "source/core/app_service.h"
#include "source/ui/particle_lab_page.xaml.h"

NS_IMPLEMENT_REFLECTION(retron::particle_lab_page_view_model, "retron.particle_lab_page_view_model")
{
    NsProp("close_command", &retron::particle_lab_page_view_model::close_command);
    NsProp("rebuild_resources_command", &retron::particle_lab_page_view_model::rebuild_resources_command);
    NsProp("particle_effects", &retron::particle_lab_page_view_model::particle_effects);
    NsProp("selected_particle_effect", &retron::particle_lab_page_view_model::selected_particle_effect, &retron::particle_lab_page_view_model::selected_particle_effect);
}

retron::particle_lab_page_view_model::particle_lab_page_view_model()
    : close_command(Noesis::MakePtr<ff::ui::delegate_command>(std::bind(&retron::particle_lab_page_view_model::debug_command, this, commands::ID_DEBUG_HIDE_UI)))
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
    if (selected_particle_effect_ != value)
    {
        this->selected_particle_effect_.Reset(value);
        this->property_changed("selected_particle_effect");
    }
}

void retron::particle_lab_page_view_model::init_particle_effects()
{
    int selected_index = this->particle_effects->IndexOf(this->selected_particle_effect());
    this->particle_effects->Clear();

    ff::dict level_particles_dict = ff::auto_resource_value("level_particles").value()->get<ff::dict>();
    for (std::string_view name : level_particles_dict.child_names())
    {
        std::string name_str = std::string(name);
        this->name_to_effect.try_emplace(name, retron::particles::effect_t(level_particles_dict.get(name)));
        this->particle_effects->Add(Noesis::Boxing::Box(name_str.c_str()));
    }

    if (selected_index >= 0 && selected_index < this->particle_effects->Count())
    {
        this->selected_particle_effect(this->particle_effects->Get(selected_index));
    }
    else
    {
        this->selected_particle_effect(this->particle_effects->Count() ? this->particle_effects->Get(0) : nullptr);
    }
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
