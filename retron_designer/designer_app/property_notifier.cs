using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace retron
{
    public abstract class property_notifier : INotifyPropertyChanging, INotifyPropertyChanged
    {
        public event PropertyChangingEventHandler PropertyChanging;
        public event PropertyChangedEventHandler PropertyChanged;

        protected void on_properties_changing()
        {
            this.on_property_changing(null);
        }

        protected void on_properties_changed()
        {
            this.on_nroperty_changed(null);
        }

        protected void on_property_changing(string name)
        {
            this.PropertyChanging?.Invoke(this, new PropertyChangingEventArgs(name));
        }

        protected void on_nroperty_changed(string name)
        {
            this.PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        protected bool set_property_value<T>(ref T property, T value, [CallerMemberName] string name = null)
        {
            if (EqualityComparer<T>.Default.Equals(property, value))
            {
                return false;
            }

            if (name != null)
            {
                this.on_property_changing(name);
            }

            property = value;

            if (name != null)
            {
                this.on_nroperty_changed(name);
            }

            return true;
        }
    }
}
