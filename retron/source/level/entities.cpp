#include "pch.h"
#include "source/core/game_spec.h"
#include "source/level/components.h"
#include "source/level/entities.h"

ff::rect_fixed retron::get_hit_box_spec(retron::entity_type type)
{
    switch (retron::category(type))
    {
        case retron::entity_category::bullet:
            return { -8, -1, 0, 1 };

        case retron::entity_category::player:
            return { -3, -13, 4, 0 };

        case retron::entity_category::electrode:
            return { -5, -5, 6, 6 };
    }

    switch (type)
    {
        case retron::entity_type::bonus_woman:
        case retron::entity_type::bonus_man:
            return { -3, -13, 4, 0 };

        case retron::entity_type::bonus_girl:
        case retron::entity_type::bonus_boy:
            return { -3, -11, 4, 0 };

        case retron::entity_type::bonus_dog:
            return { -5, -7, 5, 0 };

        case retron::entity_type::enemy_grunt:
            return { -5, -15, 6, 0 };

        case retron::entity_type::enemy_hulk:
            return { -7, -16, 8, 0 };
    }

    return {};
}

ff::rect_fixed retron::get_bounds_box_spec(retron::entity_type type)
{
    switch (retron::category(type))
    {
        case retron::entity_category::player:
            return { -4, -14, 5, 0 };
    }

    switch (type)
    {
        case retron::entity_type::bonus_woman:
        case retron::entity_type::bonus_man:
            return { -3, -14, 4, 0 };

        case retron::entity_type::bonus_girl:
        case retron::entity_type::bonus_boy:
            return { -3, -12, 4, 0 };

        case retron::entity_type::bonus_dog:
            return { -6, -8, 5, 0 };

        case retron::entity_type::enemy_grunt:
            return { -5, -15, 6, 0 };

        case retron::entity_type::enemy_hulk:
            return { -9, -18, 10, 0 };
    }

    return retron::get_hit_box_spec(type);
}

bool retron::can_hit_box_collide(retron::entity_type type_a, retron::entity_type type_b)
{
    if (type_a > type_b)
    {
        std::swap(type_a, type_b);
    }

    switch (retron::category(type_a))
    {
        case retron::entity_category::player:
            return ff::flags::has_any(type_b, retron::entity_type::category_player_collision);

        case retron::entity_category::bullet:
            return ff::flags::has_any(type_b, retron::entity_type::category_bullet_collision);

        case retron::entity_category::bonus:
            return ff::flags::has_any(type_b, retron::entity_type::category_bonus_collision);

        case retron::entity_category::enemy:
            return ff::flags::has_any(type_b, retron::entity_type::category_enemy_collision);

        default:
            return false;
    }
}

bool retron::can_bounds_box_collide(retron::entity_type type_a, retron::entity_type type_b)
{
    // One of the two needs to be a level box
    return ff::flags::has(ff::flags::toggle(type_a, type_b), retron::entity_type::category_level);
}

retron::entities::entities(entt::registry& registry)
    : registry(registry)
{}

entt::entity retron::entities::create(retron::entity_type type)
{
    entt::entity entity = this->registry.create();
    this->registry.emplace<retron::entity_type>(entity, type);
    this->entity_created_signal.notify(entity, type);
    return entity;
}

retron::entity_type retron::entities::type(entt::entity entity) const
{
    const retron::entity_type* type = this->registry.try_get<const retron::entity_type>(entity);
    return type ? *type : retron::entity_type::none;
}

retron::entity_category retron::entities::category(entt::entity entity) const
{
    return retron::category(this->type(entity));
}

bool retron::entities::delay_delete(entt::entity entity)
{
    if (!this->deleted(entity))
    {
        this->registry.emplace<retron::comp::flag::pending_delete>(entity);
        this->entity_deleting_signal.notify(entity);
        return true;
    }

    return false;
}

bool retron::entities::deleted(entt::entity entity) const
{
    return !this->registry.valid(entity) || this->registry.all_of<retron::comp::flag::pending_delete>(entity);
}

void retron::entities::flush_delete()
{
    for (entt::entity entity : this->registry.view<retron::comp::flag::pending_delete>())
    {
        this->entity_deleted_signal.notify(entity);
        this->registry.destroy(entity);
    }
}

void retron::entities::delete_all()
{
    this->registry.each([this](entt::entity entity)
        {
            this->delay_delete(entity);
        });

    this->flush_delete();
}

ff::signal_sink<entt::entity, retron::entity_type>& retron::entities::entity_created_sink()
{
    return this->entity_created_signal;
}

ff::signal_sink<entt::entity>& retron::entities::entity_deleting_sink()
{
    return this->entity_deleting_signal;
}

ff::signal_sink<entt::entity>& retron::entities::entity_deleted_sink()
{
    return this->entity_deleted_signal;
}
