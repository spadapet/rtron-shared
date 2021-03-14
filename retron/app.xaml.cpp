#include "pch.h"
#include "app.xaml.h"
#include "source/main.h"

retron::app::app()
{
	this->InitializeComponent();
}

void retron::app::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args)
{
    if (!this->init_app)
    {
        auto page = ref new Windows::UI::Xaml::Controls::Page();
        page->Content = ref new Windows::UI::Xaml::Controls::SwapChainPanel();
        Windows::UI::Xaml::Window::Current->Content = page;

        this->init_app = std::make_unique<ff::init_app>(retron::get_app_params(), retron::get_ui_params());
    }

    Windows::UI::Xaml::Window::Current->Activate();
}
