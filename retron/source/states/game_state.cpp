#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"
#include "source/states/game_state.h"
#include "source/states/level_state.h"
#include "source/states/transition_state.h"

retron::game_state::game_state()
    : game_options_(retron::app_service::get().default_game_options())
    , difficulty_spec_(retron::app_service::get().game_spec().difficulties.at(std::string(this->game_options_.difficulty_id())))
    , level_set_spec(retron::app_service::get().game_spec().level_sets.at(this->difficulty_spec_.level_set))
    , playing_level_state(0)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::game_state::init_resources, this)));

    this->init_resources();
    this->init_input();
    this->init_players();
    this->init_level_states();
}

std::shared_ptr<ff::state> retron::game_state::advance_time()
{
    this->game_input_events->advance();

    for (auto& i : this->player_input_events)
    {
        i->advance();
    }

    ff::state::advance_time();

    switch (this->level().phase())
    {
        case retron::level_phase::ready:
            if (this->level().phase_counter() >= 160)
            {
                this->level().start();
            }
            break;

        case retron::level_phase::won:
            this->transition_to_next_level();
            break;

        case retron::level_phase::dead:
            // TODO: Check high scores, restart
            break;
    }

    return nullptr;
}

void retron::game_state::render()
{
    ff::state::render();

    retron::render_targets& targets = *retron::app_service::get().render_targets();
    ff::dx11_target_base& target = *targets.target(retron::render_target_types::palette_1);
    ff::dx11_depth& depth = *targets.depth(retron::render_target_types::palette_1);

    ff::draw_ptr draw = retron::app_service::get().draw_device().begin_draw(target, &depth, retron::constants::RENDER_RECT, retron::constants::RENDER_RECT);
    if (draw)
    {
        this->render_points_and_lives(*draw);
        this->render_overlay_text(*draw);
    }
}

size_t retron::game_state::child_state_count()
{
    return 1;
}

ff::state* retron::game_state::child_state(size_t index)
{
    return this->level_states[this->playing_level_state].second.get();
}

const retron::game_options& retron::game_state::game_options() const
{
    return this->game_options_;
}

const retron::difficulty_spec& retron::game_state::difficulty_spec() const
{
    return this->difficulty_spec_;
}

const ff::input_event_provider& retron::game_state::input_events(const retron::player& player) const
{
    return *this->player_input_events[player.index];
}

void retron::game_state::player_add_points(size_t player_index, size_t points)
{
    assert(player_index < this->game_options_.player_count());

    retron::player& player = this->players[player_index].self_or_coop();
    player.points += points;

    if (player.next_life_points && player.points >= player.next_life_points)
    {
        const size_t next = this->difficulty_spec().next_free_life;
        size_t lives = 1;

        if (next)
        {
            lives += (player.points - player.next_life_points) / next;
            player.next_life_points += lives * next;
        }
        else
        {
            player.next_life_points = 0;
        }

        player.lives += lives;
    }
}

bool retron::game_state::player_get_life(size_t player_index)
{
    assert(player_index < this->game_options_.player_count());

    retron::player& player = this->players[player_index].self_or_coop();
    if (player.lives > 0)
    {
        player.lives--;
        return true;
    }

    return false;
}

void retron::game_state::debug_restart_level()
{
    this->init_level_states();
}

void retron::game_state::init_resources()
{
    this->player_life_sprite = "sprites.player_life";
    this->game_font = "game_font";
}

void retron::game_state::init_input()
{
    ff::auto_resource<ff::input_mapping> game_input_mapping = "game_controls";
    std::array<ff::auto_resource<ff::input_mapping>, constants::MAX_PLAYERS> player_input_mappings{ "player_controls", "player_controls" };
    std::vector<const ff::input_vk*> game_input_devices{ &ff::input::keyboard(), &ff::input::pointer() };
    std::array<std::vector<const ff::input_vk*>, constants::MAX_PLAYERS> player_input_devices;

    // Player specific devices
    for (size_t i = 0; i < player_input_devices.size(); i++)
    {
        // In coop, the first player must be controlled by a joystick
        if (!this->game_options_.coop() || i == 1)
        {
            player_input_devices[i].push_back(&ff::input::keyboard());
        }
    }

    // Add joysticks game-wide and player-specific
    for (size_t i = 0; i < ff::input::gamepad_count(); i++)
    {
        game_input_devices.push_back(&ff::input::gamepad(i));

        if (this->game_options_.coop())
        {
            if (i < player_input_devices.size())
            {
                player_input_devices[i].push_back(&ff::input::gamepad(i));
            }
        }
        else for (auto& input_devices : player_input_devices)
        {
            input_devices.push_back(&ff::input::gamepad(i));
        }
    }

    this->game_input_events = std::make_unique<ff::input_event_provider>(*game_input_mapping.object(), std::move(game_input_devices));

    for (size_t i = 0; i < this->player_input_events.size(); i++)
    {
        this->player_input_events[i] = std::make_unique<ff::input_event_provider>(*player_input_mappings[i].object(), std::move(player_input_devices[i]));
    }
}

static void init_player(retron::player& player, const retron::difficulty_spec& difficulty)
{
    player.lives = difficulty.lives;
    player.next_life_points = difficulty.first_free_life;
}

void retron::game_state::init_players()
{
    std::memset(this->players.data(), 0, this->players.size() * sizeof(retron::player));
    ::init_player(this->coop_player(), this->difficulty_spec_);

    for (size_t i = 0; i < this->game_options_.player_count(); i++)
    {
        retron::player& player = this->players[i];
        player.index = i;

        if (this->game_options_.coop())
        {
            player.coop = &this->coop_player();
        }
        else
        {
            ::init_player(player, this->difficulty_spec_);
        }
    }
}

void retron::game_state::init_level_states()
{
    this->level_states.clear();
    this->playing_level_state = 0;

    if (this->game_options_.coop())
    {
        std::vector<retron::player*> players;
        for (size_t i = 0; i < this->game_options_.player_count(); i++)
        {
            players.push_back(&this->players[i]);
        }

        this->add_level_state(this->players[0].self_or_coop().level, std::move(players));
    }
    else for (size_t i = 0; i < this->game_options_.player_count(); i++)
    {
        retron::player& player = this->players[i];
        this->add_level_state(player.level, std::vector<retron::player*>{ &player });
    }
}

static void render_points_and_lives(
    ff::draw_base& draw,
    const ff::sprite_font* font,
    const ff::sprite_data& sprite_data,
    ff::point_float top_middle,
    size_t points,
    size_t lives,
    bool active)
{
    const float font_x = retron::constants::FONT_SIZE.cast<float>().x;
    const DirectX::XMFLOAT4 color = ff::palette_index_to_color(active
        ? retron::colors::ACTIVE_STATUS
        : retron::colors::INACTIVE_STATUS);

    // Points
    {
        char points_str[_MAX_ITOSTR_BASE10_COUNT];
        ::_itoa_s(static_cast<int>(points), points_str, 10);
        size_t points_len = std::strlen(points_str);
        ff::transform points_pos(ff::point_float(top_middle.x - font_x * points_len, top_middle.y), ff::point_float(1, 1), 0, color);
        font->draw_text(&draw, std::string_view(points_str, points_len), points_pos, ff::color::none());
    }

    // Lives

    for (size_t i = 0; i < retron::constants::MAX_RENDER_LIVES && i < lives; i++)
    {
        draw.draw_sprite(sprite_data, ff::transform(ff::point_float(top_middle.x + font_x * i, top_middle.y)));
    }

    if (lives > retron::constants::MAX_RENDER_LIVES)
    {
        font->draw_text(&draw, "+", ff::transform(ff::point_float(top_middle.x + font_x * retron::constants::MAX_RENDER_LIVES, top_middle.y), ff::point_float(1, 1), 0, color), ff::color::none());
    }
}

void retron::game_state::render_points_and_lives(ff::draw_base& draw)
{
    for (size_t i = 0; i < (!this->game_options_.coop() ? this->game_options_.player_count() : 1); i++)
    {
        const retron::player& player = this->players[i].self_or_coop();
        ff::palette_base& palette = retron::app_service::get().player_palette(i);
        draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

        ::render_points_and_lives(draw, this->game_font.object().get(), this->player_life_sprite->sprite_data(),
            retron::constants::PLAYER_STATUS_POS[i].cast<float>(), player.points, player.lives, this->playing_level_state == i);

        draw.pop_palette_remap();
    }
}

void retron::game_state::render_overlay_text(ff::draw_base& draw)
{
    retron::level_phase phase = this->level().phase();
    const retron::player& player = this->level_state().player(0).self_or_coop();
    char text_buffer[64];
    std::string_view text{};

    if (phase == retron::level_phase::ready)
    {
        size_t counter = this->level().phase_counter();
        if (counter >= 20 && counter < 80)
        {
            if (this->game_options_.coop())
            {
                strcpy_s(text_buffer, "READY PLAYERS");
            }
            else if (this->game_options_.player_count() == 1)
            {
                strcpy_s(text_buffer, "READY PLAYER");
            }
            else
            {
                sprintf_s(text_buffer, "PLAYER %lu", player.index + 1);
            }

            text = text_buffer;
        }
        else if (counter >= 100 && counter < 160)
        {
            sprintf_s(text_buffer, "LEVEL %lu", player.level + 1);
            text = text_buffer;
        }
    }
    else if (phase == retron::level_phase::dead)
    {
        text = "GAME OVER";
    }

    if (text.size())
    {
        ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
        draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

        const ff::point_float scale(2, 2);
        ff::point_float size = this->game_font->measure_text(text, scale);
        this->game_font->draw_text(&draw, text,
            ff::transform(retron::constants::RENDER_RECT.center().cast<float>() - size / 2.0f, scale, 0, ff::palette_index_to_color(retron::colors::PLAYER)),
            ff::palette_index_to_color(224));

        draw.pop_palette_remap();
    }
}

void retron::game_state::add_level_state(size_t level_index, std::vector<retron::player*>&& players)
{
    retron::level_spec level_spec = this->level_spec(level_index);
    auto level_state = std::make_shared<retron::level_state>(*this, std::move(level_spec), std::move(players));
    this->level_states.push_back(std::make_pair(level_state, std::make_shared<ff::state_wrapper>(level_state)));
}

const retron::level_spec& retron::game_state::level_spec(size_t level_index)
{
    size_t level = level_index % this->level_set_spec.levels.size();
    const std::string& level_id = this->level_set_spec.levels[level];
    return retron::app_service::get().game_spec().levels.at(level_id);
}

void retron::game_state::transition_to_next_level()
{
    auto& pair = this->level_states[this->playing_level_state];
    auto old_state = pair.first;

    retron::player& player = old_state->players().front()->self_or_coop();
    retron::level_spec level_spec = this->level_spec(++player.level);

    pair.first = std::make_shared<retron::level_state>(*this, std::move(level_spec), old_state->players());
    *pair.second = std::make_shared<retron::transition_state>(old_state, pair.first, "transition_bg_2.png");
}

retron::level& retron::game_state::level() const
{
    return this->level_state().level();
}

retron::level_state& retron::game_state::level_state() const
{
    return *this->level_states[this->playing_level_state].first;
}

retron::player& retron::game_state::coop_player()
{
    return this->players.back();
}
