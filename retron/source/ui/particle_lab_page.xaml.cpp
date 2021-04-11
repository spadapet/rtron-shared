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

retron::particles::effect_t* retron::particle_lab_page_view_model::find_effect(std::string_view name)
{
    auto i = this->name_to_effect.find(name);
    return (i != this->name_to_effect.cend()) ? &i->second : nullptr;
}

void retron::particle_lab_page_view_model::init_particle_effects()
{
    int selected_index = this->particle_effects->IndexOf(this->selected_particle_effect());
    this->particle_effects->Clear();
    this->name_to_effect.clear();

    ff::dict level_particles_dict = ff::auto_resource_value("level_particles").value()->get<ff::dict>();
    for (std::string_view name : level_particles_dict.child_names(true))
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

ff::signal_sink<int, ff::point_float, std::string_view, retron::particles::effect_t&>& retron::particle_lab_page::clicked_sink()
{
    return this->clicked_signal;
}

bool retron::particle_lab_page::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
    NS_CONNECT_EVENT(Noesis::Grid, MouseDown, on_mouse_down);
    return false;
}

void retron::particle_lab_page::on_mouse_down(Noesis::BaseComponent* sender, const Noesis::MouseButtonEventArgs& args)
{
    int button;
    switch (args.changedButton)
    {
        case Noesis::MouseButton::MouseButton_Left:
            button = VK_LBUTTON;
            break;

        case Noesis::MouseButton::MouseButton_Right:
            button = VK_RBUTTON;
            break;

        default:
            return;
    }

    Noesis::BaseComponent* selected = this->view_model_->selected_particle_effect();
    if (selected && Noesis::Boxing::CanUnbox<Noesis::String>(selected))
    {
        ff::point_float pos(args.position.x, args.position.y);
        std::string_view name(Noesis::Boxing::Unbox<Noesis::String>(selected).Str());
        retron::particles::effect_t* effect = this->view_model_->find_effect(name);

        if (effect)
        {
            this->clicked_signal.notify(button, pos, name, *effect);
        }
    }
}
