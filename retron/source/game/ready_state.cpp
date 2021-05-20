#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/options.h"
#include "source/game/ready_state.h"
#include "source/level/level.h"

static const size_t READY_START_PLAYER = 20;
static const size_t READY_END_PLAYER = 80;
static const size_t READY_START_LEVEL = 100;
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
    return (++this->counter >= ::READY_END_ALL) ? this->under_state : nullptr;
}

void retron::ready_state::render()
{
    this->under_state->render();

    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        const retron::player& player = this->level->players().front()->self_or_coop();
        char text_buffer[64];
        std::string_view text{};

        if (this->counter >= ::READY_START_PLAYER && this->counter < ::READY_END_PLAYER)
        {
            if (this->game_service.game_options().coop())
            {
                strcpy_s(text_buffer, "READY PLAYERS");
            }
            else if (this->game_service.game_options().player_count() == 1)
            {
                strcpy_s(text_buffer, "READY PLAYER");
            }
            else
            {
                sprintf_s(text_buffer, "PLAYER %zu", player.index + 1);
            }

            text = text_buffer;
        }
        else if (this->counter >= ::READY_START_LEVEL && this->counter < ::READY_END_LEVEL)
        {
            sprintf_s(text_buffer, "LEVEL %zu", player.level + 1);
            text = text_buffer;
        }

        if (text.size())
        {
            ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
            draw->push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            const ff::point_float scale(2, 2);
            ff::point_float size = this->game_font->measure_text(text, scale);
            this->game_font->draw_text(draw, text,
                ff::transform(retron::constants::RENDER_RECT.center().cast<float>() - size / 2.0f, scale, 0, ff::palette_index_to_color(retron::colors::ACTIVE_STATUS)),
                ff::palette_index_to_color(224));

            draw->pop_palette_remap();
        }
    }
}
