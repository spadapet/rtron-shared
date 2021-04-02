using System.Windows.Controls;
using System.Windows.Input;

namespace retron
{
    public class particle_lab_page_view_model : property_notifier
    {
        public ICommand close_command => null;
    }

    public partial class particle_lab_page : UserControl
    {
        public particle_lab_page_view_model view_model { get; } = new particle_lab_page_view_model();

        public particle_lab_page()
        {
            this.InitializeComponent();
        }
    }
}
