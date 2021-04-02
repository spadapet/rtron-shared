using System.Windows.Controls;
using System.Windows.Input;

namespace retron
{
    public class debug_page_view_model : property_notifier
    {
        public ICommand restart_level_command => null;
        public ICommand restart_game_command => null;
        public ICommand rebuild_resources_command => null;
        public ICommand particle_lab_command => null;
        public ICommand close_debug_command => null;
    }

    public partial class debug_page : UserControl
    {
        public debug_page_view_model view_model { get; } = new debug_page_view_model();

        public debug_page()
        {
            this.InitializeComponent();
        }
    }
}
