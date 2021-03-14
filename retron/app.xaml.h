#pragma once
#if UWP_APP

#include "app.g.h"

namespace retron
{
	ref class app sealed
	{
	public:
		app();

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;

	private:
		std::unique_ptr<ff::init_app> init_app;
	};
}

#endif
