#include "pch.h"
#include "source/core/app_service.h"
#include "source/game/game_over_state.h"
#include "source/game/game_state.h"
#include "source/game/high_score_state.h"
#include "source/game/ready_state.h"
#include "source/game/score_state.h"
#include "source/level/level.h"
#include "source/states/transition_state.h"

retron::game_state::game_state()
    : game_options_(retron::app_service::get().default_game_options())
    , difficulty_spec_(retron::app_service::get().game_spec().difficulties.at(std::string(this->game_options_.difficulty_id())))
    , level_set_spec(retron::app_service::get().game_spec().level_sets.at(this->difficulty_spec_.level_set))
    , playing_index(0)
    , showed_player_ready(false)
{
    this->init_input();
    this->init_players();
    this->init_playing_states();
}

std::shared_ptr<ff::state> retron::game_state::advance_time()
{
    this->game_input_events->advance();

    for (auto& i : this->player_input_events)
    {
        i->advance();
    }

    ff::state::advance_time();

    playing_t& playing = this->playing_states[this->playing_index];
    switch (playing.level->phase())
    {
        case retron::level_phase::ready:
            this->handle_level_ready(playing);
            break;

        case retron::level_phase::won:
            this->handle_level_won(playing);
            break;

        case retron::level_phase::dead:
            this->handle_level_dead(playing);
            break;

        case retron::level_phase::game_over:
            return this->handle_game_over(playing);
    }

    return nullptr;
}

size_t retron::game_state::child_state_count()
{
    return 1;
}

ff::state* retron::game_state::child_state(size_t index)
{
    return this->playing_states[this->playing_index].state.get();
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

void retron::game_state::player_add_points(const retron::player& level_player, size_t points)
{
    retron::player& player = this->players[level_player.index].self_or_coop();
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

bool retron::game_state::coop_take_life(const retron::player& level_player)
{
    return level_player.coop && this->player_take_life(level_player);
}

bool retron::game_state::player_take_life(const retron::player& level_player)
{
    retron::player& player = this->players[level_player.index].self_or_coop();
    if (player.lives > 0)
    {
        player.lives--;
        return true;
    }

    return false;
}

void retron::game_state::debug_restart_level()
{
    this->init_playing_states();
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
    player.lives = difficulty.lives ? (difficulty.lives - 1) : 0;
    player.next_life_points = difficulty.first_free_life;
}

void retron::game_state::init_players()
{
    std::memset(this->players.data(), 0, this->players.size() * sizeof(retron::player));

    retron::player& coop_player = this->players.back();
    ::init_player(coop_player, this->difficulty_spec_);

    for (size_t i = 0; i < this->game_options_.player_count(); i++)
    {
        retron::player& player = this->players[i];
        player.index = i;

        if (this->game_options_.coop())
        {
            player.coop = &coop_player;
        }
        else
        {
            ::init_player(player, this->difficulty_spec_);
        }
    }
}

void retron::game_state::init_playing_states()
{
    this->playing_states.clear();
    this->playing_index = 0;

    if (this->game_options_.coop())
    {
        std::vector<retron::player*> players;
        for (size_t i = 0; i < this->game_options_.player_count(); i++)
        {
            players.push_back(&this->players[i]);
        }

        this->playing_states.push_back(this->create_playing(players));
    }
    else for (size_t i = 0; i < this->game_options_.player_count(); i++)
    {
        this->playing_states.push_back(this->create_playing(std::vector<retron::player*>{ &this->players[i] }));
    }
}

retron::player& retron::game_state::playing_t::player_or_coop() const
{
    return this->players.front()->self_or_coop();
}

void retron::game_state::handle_level_ready(playing_t& playing)
{
    playing.level->start();

    if (!this->showed_player_ready)
    {
        this->showed_player_ready = true;
        *playing.state = std::make_shared<retron::ready_state>(*this, playing.level, playing.state->unwrap());
    }
}

void retron::game_state::handle_level_won(playing_t& playing)
{
    playing.player_or_coop().level++;

    playing_t new_playing = this->create_playing(playing.players);
    *new_playing.state = std::make_shared<retron::transition_state>(playing.state, new_playing.state->unwrap(), "transition_bg_2.png");

    playing = std::move(new_playing);
}

void retron::game_state::handle_level_dead(playing_t& playing)
{
    // Either game over or ready to play again
    if (this->player_take_life(*playing.players.front()))
    {
        playing.level->restart();
    }
    else
    {
        for (retron::player* player : playing.players)
        {
            player->game_over = true;
        }

        playing.level->stop();
    }

    // Find the next player, which might be the current one for a single player game
    for (size_t i = 0; i < this->playing_states.size(); i++)
    {
        size_t check_index = (this->playing_index + 1 + i) % this->playing_states.size();
        if (this->playing_states[check_index].level->phase() == retron::level_phase::ready)
        {
            this->showed_player_ready = (this->playing_index == check_index);
            this->playing_index = check_index;
            break;
        }
    }
}

std::shared_ptr<ff::state> retron::game_state::handle_game_over(playing_t& playing)
{
    std::shared_ptr<ff::state> next_state = std::make_shared<retron::game_over_state>(this->shared_from_this());

    for (size_t i = !this->game_options_.coop() ? this->players.size() : 1; i > 0; i--)
    {
        retron::player& player = this->players[i - 1].self_or_coop();
        next_state = std::make_shared<retron::high_score_state>(this->game_options_, player, next_state);
    }

    return next_state;
}

const retron::level_spec& retron::game_state::level_spec(size_t level_index)
{
    const size_t size = this->level_set_spec.levels.size();
    const size_t loop_start = this->level_set_spec.loop_start;
    const size_t level = (level_index >= size) ? (level_index - size) % (size - loop_start) + loop_start : level_index;
    const std::string& level_id = this->level_set_spec.levels[level];

    return retron::app_service::get().game_spec().levels.at(level_id);
}

retron::game_state::playing_t retron::game_state::create_playing(const std::vector<retron::player*>& players)
{
    std::vector<const retron::player*> const_all_players;
    for (size_t i = 0; i < (!this->game_options_.coop() ? this->game_options_.player_count() : 1); i++)
    {
        const_all_players.push_back(&this->players[i].self_or_coop());
    }

    const std::vector<const retron::player*> const_players(players.cbegin(), players.cend());
    auto level = std::make_shared<retron::level>(*this, this->level_spec(players.front()->self_or_coop().level), const_players);
    auto scores = std::make_shared<retron::score_state>(const_all_players, players.front()->index);
    auto states = std::make_shared<ff::state_list>(ff::state_list{ level, scores });

    return playing_t{ players, level, states->wrap() };
}
