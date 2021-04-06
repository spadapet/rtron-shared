#include "pch.h"
#include "source/core/app_service.h"
#include "source/states/game_state.h"
#include "source/states/level_state.h"

retron::game_state::game_state()
    : game_options_(retron::app_service::get().default_game_options())
    , difficulty_spec_(retron::app_service::get().game_spec().difficulties.at(std::string(this->game_options_.difficulty_id())))
    , level_set_spec(retron::app_service::get().game_spec().level_sets.at(this->difficulty_spec_.level_set))
    , playing_level_state(0)
{
    this->init_input();
    this->init_players();
    this->init_level();
}

std::shared_ptr<ff::state> retron::game_state::advance_time()
{
    this->game_input_events->advance();

    for (auto& i : this->player_input_events)
    {
        i->advance();
    }

    return ff::state::advance_time();
}

void retron::game_state::render()
{
    ff::state::render();
}

size_t retron::game_state::child_state_count()
{
    return (this->playing_level_state < this->level_states.size()) ? 1 : 0;
}

ff::state* retron::game_state::child_state(size_t index)
{
    return this->level_states[index].get();
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

void retron::game_state::restart_level()
{
    this->init_level();
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

void retron::game_state::init_players()
{
    std::memset(this->players.data(), 0, this->players.size() * sizeof(retron::player));

    for (size_t i = 0; i < this->game_options_.player_count(); i++)
    {
        retron::player& player = this->players[i];

        player.coop = game_options_.coop() ? &this->coop_player() : nullptr;
        player.index = i;
        player.lives = this->difficulty_spec_.lives;
    }
}

void retron::game_state::init_level()
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

        size_t level_index = this->coop_player().level;
        const retron::level_spec& level_spec = this->level_spec(level_index);
        std::shared_ptr<retron::level_state> level_state = std::make_shared<retron::level_state>(level_index, this, level_spec, std::move(players));
        std::shared_ptr<ff::state_wrapper> level_wrapper = std::make_shared<ff::state_wrapper>(level_state);
        this->level_states.push_back(level_wrapper);
    }
    else
    {
        for (size_t i = 0; i < this->game_options_.player_count(); i++)
        {
            retron::player& player = this->players[i];
            std::vector<retron::player*> players;
            players.push_back(&player);

            size_t level_index = player.level;
            const retron::level_spec& level_spec = this->level_spec(level_index);
            std::shared_ptr<retron::level_state> level_state = std::make_shared<retron::level_state>(level_index, this, level_spec, std::move(players));
            std::shared_ptr<ff::state_wrapper> level_wrapper = std::make_shared<ff::state_wrapper>(level_state);
            this->level_states.push_back(level_wrapper);
        }
    }
}

retron::player& retron::game_state::coop_player()
{
    return this->players.back();
}

const retron::level_spec& retron::game_state::level_spec(size_t index) const
{
    size_t level = index % this->level_set_spec.levels.size();
    const std::string& level_id = this->level_set_spec.levels[level];
    return retron::app_service::get().game_spec().levels.at(level_id);
}
