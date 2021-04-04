#pragma once

#include "source/core/render_targets.h"

namespace retron
{
	class transition_state : public ff::state
	{
	public:
		transition_state(std::shared_ptr<ff::state> old_state, std::shared_ptr<ff::state> new_state, std::string_view image_resource, size_t speed = 4, size_t vertical_pixel_stop = 0);

        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render(ff::dx11_target_base& target, ff::dx11_depth& depth) override;

	private:
        retron::render_targets temp_targets;
		std::shared_ptr<ff::state> old_state;
		std::shared_ptr<ff::state> new_state;
		ff::auto_resource<ff::dx11_texture> image;
		std::shared_ptr<ff::dx11_texture> texture;
		std::shared_ptr<ff::dx11_texture> texture2;
		std::shared_ptr<ff::dx11_target_base> target;
		std::shared_ptr<ff::dx11_target_base> target2;
		std::shared_ptr<ff::dx11_depth> depth;
		ff::fixed_int counter;
		ff::fixed_int speed;
		ff::fixed_int offset_stop;
	};
}
