#pragma once

namespace retron
{
    class debug_state;
    class IAppService;

    class debug_page_view_model : public ff::ui::notify_propety_changed_base
    {
    public:
        debug_page_view_model(retron::debug_state* debug_state);

    private:
        void restart_level(Noesis::BaseComponent* param);
        void restart_game(Noesis::BaseComponent* param);
        void rebuild_resources(Noesis::BaseComponent* param);
        void close_debug(Noesis::BaseComponent* param);

        IAppService* _appService;
        retron::debug_state* debug_state;
        Noesis::Ptr<Noesis::ICommand> restart_level_command;
        Noesis::Ptr<Noesis::ICommand> restart_game_command;
        Noesis::Ptr<Noesis::ICommand> rebuild_resources_command;
        Noesis::Ptr<Noesis::ICommand> close_debug_command;

        NS_DECLARE_REFLECTION(retron::debug_page_view_model, ff::ui::notify_propety_changed_base);
    };

    class debug_page : public Noesis::UserControl
    {
    public:
        debug_page(retron::debug_state* debug_state);

        retron::debug_page_view_model* view_model() const;

    private:
        Noesis::Ptr<retron::debug_page_view_model> view_model_;

        NS_DECLARE_REFLECTION(retron::debug_page, Noesis::UserControl);
    };
}
