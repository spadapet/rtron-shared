#include "pch.h"
#include "source/core/app_service.h"
#include "source/states/game_over_state.h"

retron::game_over_state::game_over_state(std::shared_ptr<ff::state> under_state)
    : under_state(under_state)
    , game_font("game_font")
{}

std::shared_ptr<ff::state> retron::game_over_state::advance_time()
{
    return nullptr;
}

void retron::game_over_state::render()
{
    this->under_state->render();

    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        std::string_view text = "GAME OVER";

        const ff::point_float scale(2, 2);
        ff::point_float size = this->game_font->measure_text(text, scale);

        this->game_font->draw_text(draw, text,
            ff::transform(retron::constants::RENDER_RECT.center().cast<float>() - size / 3.0f, scale, 0, ff::palette_index_to_color(retron::colors::ACTIVE_STATUS)),
            ff::palette_index_to_color(224));
    }
}
