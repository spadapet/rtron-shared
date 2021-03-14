#pragma once

namespace retron
{
    class title_page_view_model : public ff::ui::notify_propety_changed_base
    {
    public:
        title_page_view_model();

        void visual_state_root(Noesis::FrameworkElement* value);
        std::shared_ptr<ff::state> pending_state() const;

        const char* players_text() const;
        const char* difficulty_text() const;
        const char* sound_text() const;
        const char* full_screen_text() const;

    private:
        void start_game_command(Noesis::BaseComponent* param);
        void players_command(Noesis::BaseComponent* param);
        void difficulty_command(Noesis::BaseComponent* param);
        void sound_command(Noesis::BaseComponent* param);
        void full_screen_command(Noesis::BaseComponent* param);
        void state_back_command(Noesis::BaseComponent* param);

        ff::signal_connection target_size_changed_connection;
        std::shared_ptr<ff::state> pending_state_;
        Noesis::FrameworkElement* visual_state_root_;
        Noesis::Ptr<Noesis::ICommand> start_game_command_;
        Noesis::Ptr<Noesis::ICommand> players_command_;
        Noesis::Ptr<Noesis::ICommand> difficulty_command_;
        Noesis::Ptr<Noesis::ICommand> sound_command_;
        Noesis::Ptr<Noesis::ICommand> full_screen_command_;
        Noesis::Ptr<Noesis::ICommand> state_back_command_;

        NS_DECLARE_REFLECTION(retron::title_page_view_model, ff::ui::notify_propety_changed_base);
    };

    class title_page : public Noesis::UserControl
    {
    public:
        title_page();

        title_page_view_model* view_model() const;

    private:
        Noesis::Ptr<title_page_view_model> view_model_;

        NS_DECLARE_REFLECTION(title_page, Noesis::UserControl);
    };
}
