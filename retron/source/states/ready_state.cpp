#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/options.h"
#include "source/level/level.h"
#include "source/states/ready_state.h"

retron::ready_state::ready_state(const retron::game_service& game_service, std::shared_ptr<retron::level> level, std::shared_ptr<ff::state> next_state)
    : game_service(game_service)
    , level(level)
    , next_state(next_state)
    , counter(0)
{
    assert(this->level->phase() == retron::level_phase::ready);

    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::ready_state::init_resources, this)));

    this->init_resources();
}

std::shared_ptr<ff::state> retron::ready_state::advance_time()
{
    if (this->level->phase() != retron::level_phase::ready || ++this->counter >= 160)
    {
        this->level->start();
        return this->next_state;
    }

    return nullptr;
}

void retron::ready_state::render()
{
    this->next_state->render();

    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        const retron::player& player = this->level->players().front()->self_or_coop();
        char text_buffer[64];
        std::string_view text{};

        if (this->counter >= 20 && this->counter < 80)
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
                sprintf_s(text_buffer, "PLAYER %lu", player.index + 1);
            }

            text = text_buffer;
        }
        else if (this->counter >= 100 && this->counter < 160)
        {
            sprintf_s(text_buffer, "LEVEL %lu", player.level + 1);
            text = text_buffer;
        }

        if (text.size())
        {
            ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
            draw->push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            const ff::point_float scale(2, 2);
            ff::point_float size = this->game_font->measure_text(text, scale);
            this->game_font->draw_text(draw, text,
                ff::transform(retron::constants::RENDER_RECT.center().cast<float>() - size / 2.0f, scale, 0, ff::palette_index_to_color(retron::colors::PLAYER)),
                ff::palette_index_to_color(224));

            draw->pop_palette_remap();
        }
    }
}

void retron::ready_state::init_resources()
{
    this->game_font = "game_font";
}
