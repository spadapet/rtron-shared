#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/options.h"
#include "source/game/ready_state.h"
#include "source/level/level.h"

static const size_t READY_START_PLAYER = 20;
static const size_t READY_END_PLAYER = 160;
static const size_t ADVANCE_UNDER_START = 140;
static const size_t READY_START_LEVEL = 90;
static const size_t READY_END_LEVEL = 160;
static const size_t READY_END_ALL = ::READY_END_LEVEL;

retron::ready_state::ready_state(const retron::game_service& game_service, std::shared_ptr<retron::level> level, std::shared_ptr<ff::state> under_state)
    : game_service(game_service)
    , level(level)
    , under_state(under_state)
    , game_font("game_font")
    , counter(0)
{}

std::shared_ptr<ff::state> retron::ready_state::advance_time()
{
    if (++this->counter >= ADVANCE_UNDER_START)
    {
        this->under_state->advance_time();
    }

    return (this->counter >= ::READY_END_ALL) ? this->under_state : nullptr;
}

void retron::ready_state::render()
{
    this->under_state->render();

    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        const retron::player& player = this->level->players().front()->self_or_coop();

        char player_buffer[64];
        char level_buffer[64];
        std::string_view player_text{};
        std::string_view level_text{};

        if (this->counter >= ::READY_START_PLAYER && this->counter < ::READY_END_PLAYER)
        {
            if (this->game_service.game_options().coop() || this->game_service.game_options().player_count() == 1)
            {
                strcpy_s(player_buffer, "READY");
            }
            else
            {
                sprintf_s(player_buffer, "PLAYER %zu", player.index + 1);
            }

            player_text = player_buffer;
        }

        if (this->counter >= ::READY_START_LEVEL && this->counter < ::READY_END_LEVEL)
        {
            sprintf_s(level_buffer, "LEVEL %zu", player.level + 1);
            level_text = level_buffer;
        }

        if (player_text.size() || level_text.size())
        {
            ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
            draw->push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            const ff::point_float scale(2, 2);
            DirectX::XMFLOAT4 color = ff::palette_index_to_color(retron::colors::ACTIVE_STATUS);

            if (this->counter >= ::ADVANCE_UNDER_START)
            {
                color.w -= (this->counter - ::ADVANCE_UNDER_START) / static_cast<float>(::READY_END_ALL - ::ADVANCE_UNDER_START);
            }

            if (player_text.size())
            {
                ff::point_float size = this->game_font->measure_text(player_text, scale);
                ff::point_float top_left = retron::constants::RENDER_RECT.center().cast<float>() - ff::point_float(size.x / 2.0f, size.y);
                this->game_font->draw_text(draw, player_text, ff::transform(top_left, scale, 0, color), ff::palette_index_to_color(224));
            }

            if (level_text.size())
            {
                ff::point_float size = this->game_font->measure_text(level_text, scale);
                ff::point_float top_left = retron::constants::RENDER_RECT.center().cast<float>() - ff::point_float(size.x / 2.0f, -size.y);
                this->game_font->draw_text(draw, level_text, ff::transform(top_left, scale, 0, color), ff::palette_index_to_color(224));
            }

            draw->pop_palette_remap();
        }
    }
}
