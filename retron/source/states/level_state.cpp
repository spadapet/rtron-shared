#include "pch.h"
#include "source/states/level_state.h"

retron::level_state::level_state(retron::game_service* game_service, const retron::level_spec& level_spec, std::vector<retron::player*>&& players)
    : game_service_(game_service)
    , level_spec_(level_spec)
    , players(std::move(players))
    , targets(retron::render_target_types::palette_1)
    , level(*this)
{}

std::shared_ptr<ff::state> retron::level_state::advance_time()
{
    this->level.advance(this->camera());
    return nullptr;
}

void retron::level_state::render(ff::dx11_target_base& target, ff::dx11_depth& depth)
{
    this->targets.clear();

    this->level.render(
        *this->targets.target(retron::render_target_types::palette_1),
        *this->targets.depth(retron::render_target_types::palette_1),
        constants::RENDER_RECT,
        this->camera());

    this->targets.render(target);
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

ff::rect_fixed retron::level_state::camera()
{
    return ff::rect_fixed(constants::RENDER_RECT);
}
