#include "pch.h"
#include "source/level/components.h"
#include "source/level/position.h"

retron::position::position(entt::registry& registry)
    : registry(registry)
{}

void retron::position::set(entt::entity entity, const ff::point_fixed& value)
{
    this->registry.emplace_or_replace<retron::comp::position>(entity, value);
}

ff::point_fixed retron::position::get(entt::entity entity)
{
    retron::comp::position* c = this->registry.try_get<retron::comp::position>(entity);
    return c ? c->position : ff::point_fixed(0, 0);
}

void retron::position::velocity(entt::entity entity, const ff::point_fixed& value)
{
    this->registry.emplace_or_replace<retron::comp::velocity>(entity, value);
}

ff::point_fixed retron::position::velocity(entt::entity entity)
{
    retron::comp::velocity* c = this->registry.try_get<retron::comp::velocity>(entity);
    return c ? c->velocity : ff::point_fixed(0, 0);
}

void retron::position::direction(entt::entity entity, const ff::point_fixed& value)
{
    ff::point_fixed value_canon = retron::helpers::canon_dir(value);
    this->registry.emplace_or_replace<retron::comp::direction>(entity, value_canon);
}

const ff::point_fixed retron::position::direction(entt::entity entity)
{
    retron::comp::direction* c = this->registry.try_get<retron::comp::direction>(entity);
    return c ? c->direction : ff::point_fixed(0, 1);
}

void retron::position::scale(entt::entity entity, const ff::point_fixed& value)
{
    this->registry.emplace_or_replace<retron::comp::scale>(entity, value);
}

ff::point_fixed retron::position::scale(entt::entity entity)
{
    retron::comp::scale* c = this->registry.try_get<retron::comp::scale>(entity);
    return c ? c->scale : ff::point_fixed(1, 1);
}

void retron::position::rotation(entt::entity entity, ff::fixed_int value)
{
    this->registry.emplace_or_replace<retron::comp::rotation>(entity, value);
}

ff::fixed_int retron::position::rotation(entt::entity entity)
{
    retron::comp::rotation* c = this->registry.try_get<retron::comp::rotation>(entity);
    return c ? c->rotation : 0;
}

void retron::position::render_debug(ff::draw_base& draw)
{
    for (auto [entity, pc] : this->registry.view<retron::comp::position>().each())
    {
        draw.draw_palette_filled_rectangle(ff::rect_fixed(pc.position + ff::point_fixed(-1, -1), pc.position + ff::point_fixed(1, 1)), 230);
    }
}
