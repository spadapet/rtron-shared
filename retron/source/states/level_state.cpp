#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"
#include "source/states/level_state.h"

retron::level_state::level_state(retron::game_service& game_service, retron::level_spec&& level_spec, std::vector<retron::player*>&& players)
    : game_service_(game_service)
    , players_(std::move(players))
    , level_(*this, std::move(level_spec))
{}

retron::level& retron::level_state::level()
{
    return this->level_;
}

std::vector<retron::player*> retron::level_state::players() const
{
    return this->players_;
}

std::shared_ptr<ff::state> retron::level_state::advance_time()
{
    this->level_.advance(constants::RENDER_RECT);
    return nullptr;
}

void retron::level_state::render()
{
    retron::render_targets& targets = *retron::app_service::get().render_targets();
    ff::dx11_target_base& target = *targets.target(retron::render_target_types::palette_1);
    ff::dx11_depth& depth = *targets.depth(retron::render_target_types::palette_1);

    this->level_.render(target, depth, constants::RENDER_RECT, constants::RENDER_RECT);
}

retron::game_service& retron::level_state::game_service() const
{
    return this->game_service_;
}

size_t retron::level_state::player_count() const
{
    return this->players_.size();
}

const retron::player& retron::level_state::player(size_t index_in_level) const
{
    return *this->players_[index_in_level];
}
