#pragma once

#include "source/core/level_service.h"
#include "source/level/level.h"

namespace retron
{
    struct level_spec;

    class level_state : public ff::state, public retron::level_service
    {
    public:
        level_state(retron::game_service& game_service, retron::level_spec&& level_spec, std::vector<retron::player*>&& players);

        retron::level& level();
        std::vector<retron::player*> players() const;

        // state
        virtual std::shared_ptr<ff::state> advance_time() override;
        virtual void render() override;

        // level_service
        virtual retron::game_service& game_service() const override;
        virtual size_t player_count() const override;
        virtual const retron::player& player(size_t index_in_level) const override;

    private:
        retron::game_service& game_service_;
        std::vector<retron::player*> players_;
        retron::level level_; // must be last
    };
}
