#pragma once

#include "source/core/level_service.h"
#include "source/core/game_spec.h"
#include "source/level/level.h"

namespace retron
{
    class level_state : public ff::state, public retron::level_service
    {
    public:
        level_state(size_t level_index, retron::game_service* game_service, const retron::level_spec& level_spec, std::vector<retron::player*>&& players);

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

        // level_service
        virtual const retron::game_options& game_options() const override;
        virtual const retron::difficulty_spec& difficulty_spec() const override;
        virtual const ff::input_event_provider& input_events(const retron::player& player) const override;
        virtual size_t level_index() const override;
        virtual const retron::level_spec& level_spec() const override;
        virtual size_t player_count() const override;
        virtual retron::player& player(size_t index) const override;
        virtual retron::player& player_or_coop(size_t index) const override;

    private:
        retron::game_service* game_service_;
        retron::level_spec level_spec_;
        std::vector<retron::player*> players;
        size_t level_index_;
        retron::level level; // must be last
    };
}
