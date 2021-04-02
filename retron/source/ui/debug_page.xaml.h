#pragma once

namespace retron
{
    class debug_page_view_model : public ff::ui::notify_propety_changed_base
    {
    public:
        debug_page_view_model();

    private:
        void debug_command(size_t command_id);

        Noesis::Ptr<Noesis::ICommand> restart_level_command;
        Noesis::Ptr<Noesis::ICommand> restart_game_command;
        Noesis::Ptr<Noesis::ICommand> rebuild_resources_command;
        Noesis::Ptr<Noesis::ICommand> particle_lab_command;
        Noesis::Ptr<Noesis::ICommand> close_debug_command;

        NS_DECLARE_REFLECTION(retron::debug_page_view_model, ff::ui::notify_propety_changed_base);
    };

    class debug_page : public Noesis::UserControl
    {
    public:
        debug_page();

        retron::debug_page_view_model* view_model() const;

    private:
        Noesis::Ptr<retron::debug_page_view_model> view_model_;

        NS_DECLARE_REFLECTION(retron::debug_page, Noesis::UserControl);
    };
}
