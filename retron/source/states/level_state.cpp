#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/render_targets.h"
#include "source/states/level_state.h"

retron::level_state::level_state(size_t level_index, const retron::game_service& game_service, const retron::level_spec& level_spec, std::vector<retron::player*>&& players)
    : game_service_(game_service)
    , level_spec_(level_spec)
    , players(std::move(players))
    , level_index_(level_index)
    , level(*this)
{
    this->connections.emplace_front(this->level.player_points_sink().connect(std::bind(&retron::level_state::add_player_points, this, std::placeholders::_1, std::placeholders::_2)));
}

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

const retron::game_service& retron::level_state::game_service() const
{
    return this->game_service_;
}

size_t retron::level_state::level_index() const
{
    return this->level_index_;
}

const retron::level_spec& retron::level_state::level_spec() const
{
    return this->level_spec_;
}

size_t retron::level_state::player_count() const
{
    return this->players.size();
}

const retron::player& retron::level_state::player(size_t index) const
{
    return *this->players[index];
}

const retron::player& retron::level_state::player_or_coop(size_t index) const
{
    const retron::player& player = this->player(index);
    return player.coop ? *player.coop : player;
}

void retron::level_state::add_player_points(size_t player_index, size_t points)
{
    if (player_index < this->player_count())
    {
        retron::player& temp_player = *this->players[player_index];
        retron::player& player = temp_player.coop ? *temp_player.coop : temp_player;
        player.points += points;

        if (player.next_life_points && player.points >= player.next_life_points)
        {
            const size_t next = this->game_service_.difficulty_spec().next_free_life;
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
}
