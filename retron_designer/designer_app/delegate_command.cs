using System;
using System.Windows.Input;

namespace retron
{
    public sealed class delegate_command : property_notifier, ICommand
    {
        public event EventHandler CanExecuteChanged;

        private bool? canExecute;
        private readonly Action<object> executeAction;
        private readonly Func<object, bool> canExecuteFunc;

        public delegate_command(Action executeAction, Func<bool> canExecuteFunc = null)
        {
            this.executeAction = (object arg) => executeAction?.Invoke();
            this.canExecuteFunc = (object arg) => canExecuteFunc?.Invoke() ?? true;
        }

        public delegate_command(Action<object> executeAction = null, Func<object, bool> canExecuteFunc = null)
        {
            this.executeAction = executeAction;
            this.canExecuteFunc = canExecuteFunc;
        }

        public void UpdateCanExecute()
        {
            this.canExecute = null;
            this.CanExecuteChanged?.Invoke(this, EventArgs.Empty);
            this.on_nroperty_changed(nameof(this.CanExecute));
        }

        public bool CanExecute
        {
            get
            {
                return ((ICommand)this).CanExecute(null);
            }
        }

        bool ICommand.CanExecute(object parameter)
        {
            if (this.canExecute == null)
            {
                this.canExecute = this.canExecuteFunc?.Invoke(parameter) ?? true;
            }

            return this.canExecute == true;
        }

        public void Execute(object parameter)
        {
            this.executeAction?.Invoke(parameter);
        }
    }
}
