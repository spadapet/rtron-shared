using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Interactivity;

namespace ff
{
    public class bool_to_visible_converter : IValueConverter
    {
        object IValueConverter.Convert(object value, Type targetType, object parameter, CultureInfo culture) => new NotImplementedException();
        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) => new NotImplementedException();
    }

    public class bool_to_collapsed_converter : IValueConverter
    {
        object IValueConverter.Convert(object value, Type targetType, object parameter, CultureInfo culture) => new NotImplementedException();
        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) => new NotImplementedException();
    }

    public class set_panel_child_focus_action : TargetedTriggerAction<UIElement>
    {
        protected override void Invoke(object parameter)
        {
            _ = this.Target?.Focus();
        }
    }
}
