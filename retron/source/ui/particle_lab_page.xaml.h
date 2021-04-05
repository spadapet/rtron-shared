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
        retron::particles::effect_t* find_effect(std::string_view name);

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
        virtual ~particle_lab_page() override;

        retron::particle_lab_page_view_model* view_model() const;
        ff::signal_sink<void>& destroyed_sink();
        ff::signal_sink<int, ff::point_float, std::string_view, retron::particles::effect_t&>& clicked_sink();

    private:
        virtual bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
        void on_mouse_down(Noesis::BaseComponent* sender, const Noesis::MouseButtonEventArgs& args);

        Noesis::Ptr<retron::particle_lab_page_view_model> view_model_;
        ff::signal<void> destroyed_signal;
        ff::signal<int, ff::point_float, std::string_view, retron::particles::effect_t&> clicked_signal;

        NS_DECLARE_REFLECTION(retron::particle_lab_page, Noesis::UserControl);
    };
}
