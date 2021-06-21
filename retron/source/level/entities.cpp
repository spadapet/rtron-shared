#include "pch.h"
#include "source/level/components.h"
#include "source/level/entity_type.h"
#include "source/level/entity_util.h"
#include "source/level/entities.h"

retron::entities::entities(entt::registry& registry)
    : registry(registry)
{}

retron::entity_type retron::entities::type(entt::entity entity) const
{
    const retron::entity_type* type = this->registry.try_get<const retron::entity_type>(entity);
    return type ? *type : retron::entity_type::none;
}

retron::entity_category retron::entities::category(entt::entity entity) const
{
    return retron::entity_util::category(this->type(entity));
}

entt::entity retron::entities::create(retron::entity_type type)
{
    entt::entity entity = this->registry.create();
    this->registry.emplace<retron::entity_type>(entity, type);
    return entity;
}

entt::entity retron::entities::create(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->registry.create();
    this->registry.emplace<retron::entity_type>(entity, type);
    this->registry.emplace<retron::comp::position>(entity, pos);
    return entity;
}

entt::entity retron::entities::create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top)
{
    return anim ? this->create_animation(std::make_shared<ff::animation_player>(anim), pos, top) : entt::null;
}

entt::entity retron::entities::create_animation(std::shared_ptr<ff::animation_player_base> anim_player, ff::point_fixed pos, bool top)
{
    if (anim_player)
    {
        entt::entity entity = this->create(top ? retron::entity_type::animation_top : retron::entity_type::animation_bottom, pos);
        this->registry.emplace<retron::comp::animation>(entity, std::move(anim_player));

        if (top)
        {
            this->registry.emplace<retron::comp::flag::render_on_top>(entity);
        }

        return entity;
    }

    return entt::null;
}

entt::entity retron::entities::create_bonus(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create(type, pos);
    this->registry.emplace<retron::comp::bonus>(entity, 0u);
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{});
    this->registry.emplace<retron::comp::flag::hulk_target>(entity);

    return entity;
}

entt::entity retron::entities::create_electrode(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create(type, pos);
    this->registry.emplace<retron::comp::electrode>(entity);

    return entity;
}

entt::entity retron::entities::create_grunt(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create(type, pos);
    this->registry.emplace<retron::comp::grunt>(entity, this->registry.size<retron::comp::grunt>(), 0u, pos);
    return entity;
}

entt::entity retron::entities::create_hulk(retron::entity_type type, const ff::point_fixed& pos, size_t group)
{
    entt::entity entity = this->create(type, pos);
    this->registry.emplace<retron::comp::hulk>(entity, this->registry.size<retron::comp::hulk>(), group, entt::null, ff::point_fixed{}, false);
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{});
    return entity;
}

entt::entity retron::entities::create_bullet(entt::entity player, ff::point_fixed pos, ff::point_fixed vel)
{
    retron::entity_type type = retron::entity_util::bullet_for_player(this->type(player));
    entt::entity entity = this->create(type, pos);

    this->registry.emplace<retron::comp::bullet>(entity);
    this->registry.emplace<retron::comp::rotation>(entity, retron::helpers::dir_to_degrees(retron::helpers::canon_dir(vel)));
    this->registry.emplace<retron::comp::velocity>(entity, vel);

    return entity;
}

bool retron::entities::delay_delete(entt::entity entity)
{
    if (!this->deleted(entity))
    {
        this->registry.emplace<retron::comp::flag::pending_delete>(entity);
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
        this->registry.destroy(entity);
    }
}

void retron::entities::delete_all()
{
    this->registry.each([this](entt::entity entity)
        {
            this->registry.emplace_or_replace<retron::comp::flag::pending_delete>(entity);
        });

    this->flush_delete();
}
