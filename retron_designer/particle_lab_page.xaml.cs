using System.Collections.Generic;
using System.Windows.Controls;
using System.Windows.Input;

namespace retron
{
    public class particle_lab_page_view_model : property_notifier
    {
        public ICommand close_command => null;
        public ICommand rebuild_resources_command => null;

        public IEnumerable<string> particle_effects => new string[]
            {
                "particles 1",
                "particles 2",
                "particles 3",
                "particles 4",
                "particles 5",
                "particles 6",
                "particles 7",
                "particles 8",
                "particles 9",
            };

        private string selected_particle_effect_ = "particles 2";
        public string selected_particle_effect
        {
            get => this.selected_particle_effect_;
            set => this.set_property_value(ref this.selected_particle_effect_, value);
        }
    }

    public partial class particle_lab_page : UserControl
    {
        public particle_lab_page_view_model view_model { get; } = new particle_lab_page_view_model();

        public particle_lab_page()
        {
            this.InitializeComponent();
        }

        private void on_mouse_down(object sender, MouseButtonEventArgs args)
        {
        }
    }
}
