#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"
#include "source/states/ui_view_state.h"

retron::ui_view_state::ui_view_state(std::shared_ptr<ff::ui_view> view)
    : ff::ui_view_state(view, nullptr, nullptr)
{
    view->size(ff::window_size{ retron::constants::RENDER_SIZE.cast<int>(), 1.0, DMDO_DEFAULT, DMDO_DEFAULT });
}

void retron::ui_view_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    retron::render_targets* targets = retron::app_service::get().render_targets();

    ff::ui_view_state::render(
        *targets->target(retron::render_target_types::rgb_pma_2),
        *targets->depth(retron::render_target_types::rgb_pma_2));
}
