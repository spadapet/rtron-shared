#include "pch.h"
#include "source/level/entities.h"

namespace
{
    struct pending_delete
    {};
}

retron::entity_box_type retron::box_type(retron::entity_type type)
{
    static retron::entity_box_type types[] =
    {
        retron::entity_box_type::none, // none
        retron::entity_box_type::player_bullet, // player_bullet
        retron::entity_box_type::player, // player
        retron::entity_box_type::bonus, // bonus_woman
        retron::entity_box_type::bonus, // bonus_man
        retron::entity_box_type::bonus, // bonus_child
        retron::entity_box_type::enemy, // grunt
        retron::entity_box_type::enemy, // hulk
        retron::entity_box_type::obstacle, // electrode
        retron::entity_box_type::none, // level_border
        retron::entity_box_type::none, // level_box
    };

    static_assert(_countof(types) == static_cast<size_t>(retron::entity_type::count));
    assert(static_cast<size_t>(type) < _countof(types));
    return types[static_cast<size_t>(type)];
}

const ff::rect_fixed& retron::get_hit_box_spec(retron::entity_type type)
{
    static ff::rect_fixed rects[] =
    {
        ff::rect_fixed(0, 0, 0, 0), // none
        ff::rect_fixed(-8, -1, 0, 1), // player_bullet
        ff::rect_fixed(-3, -10, 4, 0), // player
        ff::rect_fixed(-4, -8, 4, 0), // bonus_woman
        ff::rect_fixed(-4, -8, 4, 0), // bonus_man
        ff::rect_fixed(-4, -8, 4, 0), // bonus_child
        ff::rect_fixed(-5, -15, 6, 0), // grunt
        ff::rect_fixed(-5, -8, 5, 0), // hulk
        ff::rect_fixed(-5, -5, 6, 6), // electrode
        ff::rect_fixed(0, 0, 0, 0), // level_border
        ff::rect_fixed(0, 0, 0, 0), // level_box
    };

    static_assert(_countof(rects) == static_cast<size_t>(retron::entity_type::count));
    assert(static_cast<size_t>(type) < _countof(rects));
    return rects[static_cast<size_t>(type)];
}

const ff::rect_fixed& retron::get_bounds_box_spec(retron::entity_type type)
{
    static ff::rect_fixed rects[] =
    {
        ff::rect_fixed(0, 0, 0, 0), // none
        ff::rect_fixed(-8, -1, 0, 1), // player_bullet
        ff::rect_fixed(-4, -14, 5, 0), // player
        ff::rect_fixed(-5, -12, 5, 0), // bonus_woman
        ff::rect_fixed(-5, -12, 5, 0), // bonus_man
        ff::rect_fixed(-5, -12, 5, 0), // bonus_child
        ff::rect_fixed(-5, -15, 6, 0), // grunt
        ff::rect_fixed(-5, -12, 5, 0), // hulk
        ff::rect_fixed(-5, -5, 6, 6), // electrode
        ff::rect_fixed(0, 0, 0, 0), // level_border
        ff::rect_fixed(0, 0, 0, 0), // level_box
    };

    static_assert(_countof(rects) == static_cast<size_t>(retron::entity_type::count));
    assert(static_cast<size_t>(type) < _countof(rects));
    return rects[static_cast<size_t>(type)];
}

bool retron::can_hit_box_collide(retron::entity_box_type type_a, retron::entity_box_type type_b)
{
    if (type_a != type_b && type_a != retron::entity_box_type::none && type_b != retron::entity_box_type::none)
    {
        if (type_a > type_b)
        {
            std::swap(type_a, type_b);
        }

        switch (type_a)
        {
            case retron::entity_box_type::player:
                return type_b == retron::entity_box_type::bonus ||
                    type_b == retron::entity_box_type::enemy ||
                    type_b == retron::entity_box_type::obstacle ||
                    type_b == retron::entity_box_type::enemy_bullet ||
                    type_b == retron::entity_box_type::level;

            case retron::entity_box_type::bonus:
                return type_b == retron::entity_box_type::enemy ||
                    type_b == retron::entity_box_type::obstacle ||
                    type_b == retron::entity_box_type::enemy_bullet ||
                    type_b == retron::entity_box_type::level;

            case retron::entity_box_type::enemy:
                return type_b == retron::entity_box_type::obstacle ||
                    type_b == retron::entity_box_type::player_bullet ||
                    type_b == retron::entity_box_type::level;

            case retron::entity_box_type::obstacle:
                return type_b == retron::entity_box_type::player_bullet ||
                    type_b == retron::entity_box_type::enemy_bullet;

            case retron::entity_box_type::player_bullet:
                return type_b == retron::entity_box_type::enemy_bullet ||
                    type_b == retron::entity_box_type::level;

            case retron::entity_box_type::enemy_bullet:
                return type_b == retron::entity_box_type::level;
        }
    }

    return false;
}

bool retron::can_bounds_box_collide(retron::entity_box_type type_a, retron::entity_box_type type_b)
{
    return (type_a != type_b) && (type_a == retron::entity_box_type::level || type_b == retron::entity_box_type::level);
}

retron::entities::entities(entt::registry& registry)
    : registry(registry)
    , sort_entities_(false)
{}

entt::entity retron::entities::create(retron::entity_type type)
{
    entt::entity entity = this->registry.create();
    this->registry.emplace<retron::entity_type>(entity, type);
    this->sort_entities_ = true;
    this->entity_created_signal.notify(entity);

    return entity;
}

bool retron::entities::delay_delete(entt::entity entity)
{
    if (!this->deleted(entity))
    {
        this->registry.emplace_or_replace<::pending_delete>(entity);
        this->entity_deleting_signal.notify(entity);
        return true;
    }

    return false;
}

bool retron::entities::deleted(entt::entity entity)
{
    return this->registry.all_of<::pending_delete>(entity);
}

void retron::entities::flush_delete()
{
    for (entt::entity entity : this->registry.view<::pending_delete>())
    {
        this->registry.destroy(entity);
    }
}

size_t retron::entities::sort_entities()
{
    if (this->sort_entities_)
    {
        this->sort_entities_ = false;

        this->registry.sort<retron::entity_type>([](retron::entity_type type_a, retron::entity_type type_b)
            {
                // Since we loop backwards
                return type_a > type_b;
            });
    }

    return this->entity_count();
}

size_t retron::entities::entity_count() const
{
    return this->registry.size<retron::entity_type>();
}

entt::entity retron::entities::entity(size_t index) const
{
    return this->registry.view<retron::entity_type>().data()[index];
}

retron::entity_type retron::entities::entity_type(entt::entity entity)
{
    return this->registry.get<retron::entity_type>(entity);
}

ff::signal_sink<entt::entity>& retron::entities::entity_created_sink()
{
    return this->entity_created_signal;
}

ff::signal_sink<entt::entity>& retron::entities::entity_deleted_sink()
{
    return this->entity_deleting_signal;
}
