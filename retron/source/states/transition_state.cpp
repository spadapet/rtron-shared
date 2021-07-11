#include "pch.h"
#include "source/core/app_service.h"
#include "source/states/transition_state.h"

retron::transition_state::transition_state(std::shared_ptr<ff::state> old_state, std::shared_ptr<ff::state> new_state, std::string_view image_resource, size_t speed, size_t vertical_pixel_stop)
    : old_state(old_state)
    , new_state(new_state)
    , image(image_resource)
    , counter(0)
    , speed(static_cast<int>(speed))
    , offset_stop(static_cast<int>(vertical_pixel_stop))
{
    assert(this->new_state);

    ff::point_int size = constants::RENDER_SIZE.cast<int>();

    this->texture = std::make_shared<ff::texture>(size, DXGI_FORMAT_R8G8B8A8_UNORM);
    this->texture2 = std::make_shared<ff::texture>(size, DXGI_FORMAT_R8G8B8A8_UNORM);
    this->target = std::make_shared<ff::target_texture>(this->texture);
    this->target2 = std::make_shared<ff::target_texture>(this->texture2);
}

std::shared_ptr<ff::state> retron::transition_state::advance_time()
{
    if (this->old_state)
    {
        this->counter += 40;

        if (this->counter > constants::RENDER_WIDTH)
        {
            this->old_state = nullptr;
            this->counter = 0;
        }
    }
    else
    {
        this->counter += this->speed;

        if (this->counter >= constants::RENDER_HEIGHT - this->offset_stop)
        {
            this->new_state->advance_time();
            return this->new_state;
        }
    }

    return nullptr;
}

void retron::transition_state::render()
{
    ff::graphics::dx11_device_state().clear_target(this->target->view(), ff::color::black());
    ff::graphics::dx11_device_state().clear_target(this->target2->view(), ff::color::black());

    ff::fixed_int half_height = constants::RENDER_HEIGHT / 2;
    retron::app_service& app = retron::app_service::get();

    if (this->old_state)
    {
        app.push_render_targets(this->temp_targets);
        this->old_state->render();
        app.pop_render_targets(*this->target);

        ff::rect_fixed rect = constants::RENDER_RECT.right_edge();
        rect.left -= std::min(this->counter, constants::RENDER_WIDTH);

        ff::draw_ptr draw = app.draw_device().begin_draw(*this->target, nullptr, rect, rect);
        if (draw)
        {
            draw->draw_filled_rectangle(rect.cast<float>(), ff::color::black());
        }
    }
    else if (this->counter <= half_height)
    {
        ff::fixed_int offset = half_height - std::min(half_height, this->counter);
        ff::rect_fixed rect(offset, offset, constants::RENDER_WIDTH - offset, constants::RENDER_HEIGHT - offset);
        ff::draw_ptr draw = app.draw_device().begin_draw(*this->target, nullptr, rect, rect);
        if (draw)
        {
            draw->push_palette(&app.palette());
            draw->draw_sprite(this->image->sprite_data(), ff::transform::identity());
        }
    }
    else
    {
        // Draw gradient image
        {
            ff::draw_ptr draw = app.draw_device().begin_draw(*this->target, nullptr, constants::RENDER_RECT, constants::RENDER_RECT);
            if (draw)
            {
                draw->push_palette(&app.palette());
                draw->draw_sprite(this->image->sprite_data(), ff::transform::identity());
            }
        }

        app.push_render_targets(this->temp_targets);
        this->new_state->render();
        app.pop_render_targets(*this->target2);

        // Draw new state
        {
            ff::fixed_int offset = half_height - std::min(half_height, this->counter - half_height);
            ff::rect_fixed rect(offset, offset, constants::RENDER_WIDTH - offset, constants::RENDER_HEIGHT - offset);
            ff::draw_ptr draw = app.draw_device().begin_draw(*this->target, nullptr, rect, rect);
            if (draw)
            {
                draw->draw_sprite(this->texture2->sprite_data(), ff::transform::identity());
            }
        }
    }

    ff::draw_ptr draw = app.draw_device().begin_draw(
        *app.render_targets()->target(retron::render_target_types::rgb_pma_2),
        app.render_targets()->depth(retron::render_target_types::rgb_pma_2).get(),
        constants::RENDER_RECT, constants::RENDER_RECT);

    if (draw)
    {
        draw->draw_sprite(this->texture->sprite_data(), ff::transform::identity());
    }
}
