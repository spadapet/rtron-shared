#pragma once

namespace retron
{
    class particle_lab_page_view_model : public ff::ui::notify_propety_changed_base
    {
    public:
        particle_lab_page_view_model();

    private:
        void debug_command(size_t command_id);

        Noesis::Ptr<Noesis::ICommand> close_debug_command;

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
