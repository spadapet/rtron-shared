#pragma once

#include "source/level/particles.h"

namespace retron
{
    class particle_lab_page_view_model : public ff::ui::notify_propety_changed_base
    {
    public:
        particle_lab_page_view_model();

        Noesis::BaseComponent* selected_particle_effect() const;
        void selected_particle_effect(Noesis::BaseComponent* value);

    private:
        void init_particle_effects();
        void debug_command(size_t command_id);

        Noesis::Ptr<Noesis::ICommand> close_command;
        Noesis::Ptr<Noesis::ICommand> rebuild_resources_command;
        Noesis::Ptr<Noesis::ObservableCollection<Noesis::BaseComponent>> particle_effects;
        Noesis::Ptr<Noesis::BaseComponent> selected_particle_effect_;
        std::unordered_map<std::string_view, retron::particles::effect_t> name_to_effect;
        std::forward_list<ff::signal_connection> connections;

        NS_DECLARE_REFLECTION(retron::particle_lab_page_view_model, ff::ui::notify_propety_changed_base);
    };

    class particle_lab_page : public Noesis::UserControl
    {
    public:
        particle_lab_page();

        retron::particle_lab_page_view_model* view_model() const;

    private:
        Noesis::Ptr<retron::particle_lab_page_view_model> view_model_;

        NS_DECLARE_REFLECTION(retron::particle_lab_page, Noesis::UserControl);
    };
}
