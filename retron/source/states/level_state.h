#pragma once

#include "source/core/level_service.h"
#include "source/core/game_spec.h"
#include "source/level/level.h"

namespace retron
{
    class level_state : public ff::state, public retron::level_service
    {
    public:
        level_state(const retron::game_service& game_service, size_t level_index, retron::level_spec&& level_spec, std::vector<retron::player*>&& players);

        retron::level& level();

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

        // level_service
        virtual const retron::game_service& game_service() const override;
        virtual size_t player_count() const override;
        virtual const retron::player& player(size_t index) const override;
        virtual const retron::player& player_or_coop(size_t index) const override;

    private:
        void add_player_points(size_t player_index, size_t points);

        const retron::game_service& game_service_;
        std::forward_list<ff::signal_connection> connections;
        std::vector<retron::player*> players;
        retron::level level_; // must be last
    };
}
