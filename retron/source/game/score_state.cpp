#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/core/render_targets.h"
#include "source/game/score_state.h"

static void render_points_and_lives(
    ff::draw_base& draw,
    const ff::sprite_font* font,
    const ff::sprite_data& sprite_data,
    ff::point_float top_middle,
    const retron::player& player,
    bool active)
{
    const float font_x = retron::constants::FONT_SIZE.cast<float>().x;
    const DirectX::XMFLOAT4 color = ff::palette_index_to_color(active
        ? retron::colors::ACTIVE_STATUS
        : retron::colors::INACTIVE_STATUS);

    // Points
    {
        char points_str[_MAX_ITOSTR_BASE10_COUNT];
        ::_itoa_s(static_cast<int>(player.points), points_str, 10);
        size_t points_len = std::strlen(points_str);
        ff::transform points_pos(ff::point_float(top_middle.x - font_x * points_len, top_middle.y), ff::point_float(1, 1), 0, color);
        font->draw_text(&draw, std::string_view(points_str, points_len), points_pos, ff::color::none());
    }

    // Lives

    size_t render_lives = player.lives + ((active || player.game_over) ? 0 : 1);

    for (size_t i = 0; i < retron::constants::MAX_RENDER_LIVES && i < render_lives; i++)
    {
        draw.draw_sprite(sprite_data, ff::transform(ff::point_float(top_middle.x + font_x * i, top_middle.y)));
    }

    if (render_lives > retron::constants::MAX_RENDER_LIVES)
    {
        font->draw_text(&draw, "+", ff::transform(ff::point_float(top_middle.x + font_x * retron::constants::MAX_RENDER_LIVES, top_middle.y), ff::point_float(1, 1), 0, color), ff::color::none());
    }
}

retron::score_state::score_state(const std::vector<const retron::player*>& players, size_t active_player)
    : players(players)
    , active_player(active_player)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::score_state::init_resources, this)));

    this->init_resources();
}

void retron::score_state::render()
{
    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        for (size_t i = 0; i < this->players.size(); i++)
        {
            const retron::player& player = this->players[i]->self_or_coop();
            ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
            draw->push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            ::render_points_and_lives(*draw, this->game_font.object().get(), this->player_sprite->sprite_data(),
                retron::constants::PLAYER_STATUS_POS[i].cast<float>(), player, this->active_player == i);

            draw->pop_palette_remap();
        }
    }
}

void retron::score_state::init_resources()
{
    this->player_sprite = "sprites.player_life";
    this->game_font = "game_font";
}
