#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"

static const ff::point_int LOW_SIZE(constants::RENDER_WIDTH, constants::RENDER_HEIGHT);
static const ff::point_int HIGH_SIZE = ::LOW_SIZE * constants::RENDER_SCALE;

retron::render_targets::render_targets(retron::render_target_types targets)
    : targets(targets)
    , viewport(ff::point_int(constants::RENDER_WIDTH, constants::RENDER_HEIGHT))
    , depth_(std::make_shared<ff::dx11_depth>(::LOW_SIZE))
    , texture_rgb_pma_1(std::make_shared<ff::dx11_texture>(::LOW_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM))
    , target_rgb_pma_1(std::make_shared<ff::dx11_target_texture>(this->texture_rgb_pma_1))
    , texture_palette_1(std::make_shared<ff::dx11_texture>(::LOW_SIZE, DXGI_FORMAT_R8_UINT))
    , target_palette_1(std::make_shared<ff::dx11_target_texture>(this->texture_palette_1))
    , texture_1080(std::make_shared<ff::dx11_texture>(::HIGH_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM))
    , target_1080(std::make_shared<ff::dx11_target_texture>(this->texture_1080))
{}

void retron::render_targets::clear()
{
    if (ff::flags::has(this->targets, retron::render_target_types::palette_1))
    {
        ff::graphics::dx11_device_state().clear_target(this->target(retron::render_target_types::palette_1)->view(), ff::color::none());
    }

    if (ff::flags::has(this->targets, retron::render_target_types::rgb_pma_2))
    {
        ff::graphics::dx11_device_state().clear_target(this->target(retron::render_target_types::rgb_pma_2)->view(), ff::color::none());
    }
}

void retron::render_targets::render(ff::dx11_target_base& target)
{
    ff::graphics::dx11_device_state().clear_target(this->target_1080->view(), ff::color::none());

    ff::draw_ptr draw = retron::app_service::get().draw_device().begin_draw(*this->target_1080, nullptr, constants::RENDER_RECT_HIGH, constants::RENDER_RECT);
    if (ff::flags::has(this->targets, retron::render_target_types::palette_1))
    {
        draw->push_palette(&retron::app_service::get().palette());
        draw->draw_sprite(this->texture(retron::render_target_types::palette_1)->sprite_data(), ff::transform::identity());
    }

    if (ff::flags::has(this->targets, retron::render_target_types::rgb_pma_2))
    {
        draw->push_pre_multiplied_alpha();
        draw->draw_sprite(this->texture(retron::render_target_types::rgb_pma_2)->sprite_data(), ff::transform::identity());
    }

    ff::rect_fixed target_rect = this->viewport.view(target.size().rotated_pixel_size()).cast<ff::fixed_int>();
    draw = retron::app_service::get().draw_device().begin_draw(target, nullptr, target_rect, constants::RENDER_RECT_HIGH);
    draw->push_texture_sampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
    draw->draw_sprite(this->texture_1080->sprite_data(), ff::transform::identity());
}

const std::shared_ptr<ff::dx11_texture>& retron::render_targets::texture(retron::render_target_types target) const
{
    switch (target)
    {
        case retron::render_target_types::palette_1:
            return this->texture_palette_1;

        default:
        case retron::render_target_types::rgb_pma_2:
            return this->texture_rgb_pma_1;
    }
}

const std::shared_ptr<ff::dx11_target_base>& retron::render_targets::target(retron::render_target_types target) const
{
    switch (target)
    {
        case retron::render_target_types::palette_1:
            return this->target_palette_1;

        default:
        case retron::render_target_types::rgb_pma_2:
            return this->target_rgb_pma_1;
    }
}

const std::shared_ptr<ff::dx11_depth>& retron::render_targets::depth(retron::render_target_types target) const
{
    return this->depth_;
}
