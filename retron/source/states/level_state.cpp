#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"
#include "source/states/level_state.h"

retron::level_state::level_state(retron::game_service* game_service, const retron::level_spec& level_spec, std::vector<retron::player*>&& players)
    : game_service_(game_service)
    , level_spec_(level_spec)
    , players(std::move(players))
    , level(*this)
{}

std::shared_ptr<ff::state> retron::level_state::advance_time()
{
    this->level.advance(constants::RENDER_RECT);
    return nullptr;
}

void retron::level_state::render()
{
    retron::render_targets& targets = *retron::app_service::get().render_targets();

    this->level.render(
        *targets.target(retron::render_target_types::palette_1),
        *targets.depth(retron::render_target_types::palette_1),
        constants::RENDER_RECT,
        constants::RENDER_RECT);
}

const retron::game_options& retron::level_state::game_options() const
{
    return this->game_service_->game_options();
}

const retron::difficulty_spec& retron::level_state::difficulty_spec() const
{
    return this->game_service_->difficulty_spec();
}

const ff::input_event_provider& retron::level_state::input_events(const retron::player& player) const
{
    return this->game_service_->input_events(player);
}

const retron::level_spec& retron::level_state::level_spec() const
{
    return this->level_spec_;
}

size_t retron::level_state::player_count() const
{
    return this->players.size();
}

retron::player& retron::level_state::player(size_t index) const
{
    return *this->players[index];
}

retron::player& retron::level_state::player_or_coop(size_t index) const
{
    retron::player& player = this->player(index);
    return player.coop ? *player.coop : player;
}
