using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace retron
{
    public class title_page_view_model : property_notifier
    {
        public ICommand start_game_command => null;
        public ICommand players_command => new delegate_command((object param) => this.change_players(param is bool forward && forward));
        public ICommand difficulty_command => new delegate_command((object param) => this.change_difficulty(param is bool forward && forward));
        public ICommand sound_command => new delegate_command(this.change_sound);
        public ICommand full_screen_command => new delegate_command(this.change_full_screen);
        public ICommand state_back_command => new delegate_command(this.state_back);

        private FrameworkElement visual_state_root;
        public void set_visual_state_root(FrameworkElement visual_state_root)
        {
            this.visual_state_root = visual_state_root;
        }

        private int players = 0;
        public string players_text => (this.players + 1).ToString();
        public void change_players(bool forward = true)
        {
            this.players = forward ? (this.players + 1) % 4 : (this.players + 3) % 4;
            this.on_nroperty_changed(nameof(this.players_text));
        }

        private int difficulty = 1;
        public string difficulty_text => this.difficulty.ToString();
        public void change_difficulty(bool forward = true)
        {
            this.difficulty = (this.difficulty + (forward ? 1 : 2)) % 3;
            this.on_nroperty_changed(nameof(this.difficulty_text));
        }

        private bool sound = true;
        public string sound_text => this.sound ? "On" : "Off";
        public void change_sound()
        {
            this.sound = !this.sound;
            this.on_nroperty_changed(nameof(this.sound_text));
        }

        private bool full_screen = true;
        public string full_screen_text => this.full_screen ? "On" : "Off";
        public void change_full_screen()
        {
            this.full_screen = !this.full_screen;
            this.on_nroperty_changed(nameof(this.full_screen_text));
        }

        public void state_back()
        {
            VisualStateGroup group = VisualStateManager.GetVisualStateGroups(this.visual_state_root).Cast<VisualStateGroup>().First(i => i.Name == "main_group");
            if (string.Equals(group.CurrentState?.Name, "initial_state"))
            {
                this.quit();
            }
            else
            {
                _ = VisualStateManager.GoToElementState(this.visual_state_root, "initial_state", true);
            }
        }

        public void quit()
        {
            Window.GetWindow(this.visual_state_root).Close();
        }
    }

    partial class title_page : UserControl
    {
        public title_page_view_model view_model { get; } = new title_page_view_model();

        public title_page()
        {
            this.InitializeComponent();
            this.view_model.set_visual_state_root(this.root_panel);
        }
    }
}
