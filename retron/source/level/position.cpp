#include "pch.h"
#include "source/level/position.h"

namespace
{
    struct position_component
    {
        ff::point_fixed position;
    };

    struct velocity_component
    {
        ff::point_fixed velocity;
    };

    struct direction_component
    {
        ff::point_fixed direction;
    };

    struct scale_component
    {
        ff::point_fixed scale;
    };

    struct rotation_component
    {
        ff::fixed_int rotation;
    };
}

retron::position::position(entt::registry& registry)
    : registry(registry)
{
    this->connections.emplace_front(this->registry.on_construct<::position_component>().connect<&retron::position::position_changed>(this));
    this->connections.emplace_front(this->registry.on_update<::position_component>().connect<&retron::position::position_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<::velocity_component>().connect<&retron::position::velocity_changed>(this));
    this->connections.emplace_front(this->registry.on_update<::velocity_component>().connect<&retron::position::position_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<::direction_component>().connect<&retron::position::direction_changed>(this));
    this->connections.emplace_front(this->registry.on_update<::direction_component>().connect<&retron::position::direction_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<::scale_component>().connect<&retron::position::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_update<::scale_component>().connect<&retron::position::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<::rotation_component>().connect<&retron::position::rotation_changed>(this));
    this->connections.emplace_front(this->registry.on_update<::rotation_component>().connect<&retron::position::rotation_changed>(this));
}

void retron::position::set(entt::entity entity, const ff::point_fixed& value)
{
    if (value != this->get(entity))
    {
        this->registry.emplace_or_replace<::position_component>(entity, value);
    }
}

ff::point_fixed retron::position::get(entt::entity entity)
{
    ::position_component* c = this->registry.try_get<::position_component>(entity);
    return c ? c->position : ff::point_fixed(0, 0);
}

ff::pixel_transform retron::position::pixel_transform(entt::entity entity)
{
    ff::point_fixed pos = std::floor(this->get(entity));
    ff::point_fixed scale = this->scale(entity);
    ff::fixed_int rotation = this->rotation(entity);

    return ff::pixel_transform(pos, scale, rotation);
}

void retron::position::velocity(entt::entity entity, const ff::point_fixed& value)
{
    if (value != this->velocity(entity))
    {
        this->registry.emplace_or_replace<::velocity_component>(entity, value);
    }
}

ff::point_fixed retron::position::velocity(entt::entity entity)
{
    ::velocity_component* c = this->registry.try_get<::velocity_component>(entity);
    return c ? c->velocity : ff::point_fixed(0, 0);
}

ff::fixed_int retron::position::velocity_as_angle(entt::entity entity)
{
    ff::point_fixed vel = this->velocity(entity);
    return ff::math::radians_to_degrees(std::atan2(static_cast<float>(vel.y), static_cast<float>(vel.x)));
}

ff::fixed_int retron::position::reverse_velocity_as_angle(entt::entity entity)
{
    ff::point_fixed vel = this->velocity(entity);
    return ff::math::radians_to_degrees(std::atan2(static_cast<float>(-vel.y), static_cast<float>(-vel.x)));
}

void retron::position::direction(entt::entity entity, const ff::point_fixed& value)
{
    ff::point_fixed value_canon = retron::position::canon_direction(value);
    if (value_canon && value_canon != this->direction(entity))
    {
        this->registry.emplace_or_replace<::direction_component>(entity, value_canon);
    }
}

const ff::point_fixed retron::position::direction(entt::entity entity)
{
    ::direction_component* c = this->registry.try_get<::direction_component>(entity);
    return c ? c->direction : ff::point_fixed(0, 1);
}

ff::point_fixed retron::position::canon_direction(const ff::point_fixed& value)
{
    return ff::point_fixed(
        std::copysign(1_f, value.x) * ff::fixed_int(value.x != 0_f),
        std::copysign(1_f, value.y) * ff::fixed_int(value.y != 0_f));
}

void retron::position::scale(entt::entity entity, const ff::point_fixed& value)
{
    if (value != this->scale(entity))
    {
        this->registry.emplace_or_replace<::scale_component>(entity, value);
    }
}

ff::point_fixed retron::position::scale(entt::entity entity)
{
    ::scale_component* c = this->registry.try_get<::scale_component>(entity);
    return c ? c->scale : ff::point_fixed(1, 1);
}

void retron::position::rotation(entt::entity entity, ff::fixed_int value)
{
    if (value != this->rotation(entity))
    {
        this->registry.emplace_or_replace<::rotation_component>(entity, value);
    }
}

ff::fixed_int retron::position::rotation(entt::entity entity)
{
    ::rotation_component* c = this->registry.try_get<::rotation_component>(entity);
    return c ? c->rotation : 0;
}

ff::signal_sink<entt::entity>& retron::position::position_changed_sink()
{
    return this->position_changed_signal;
}

ff::signal_sink<entt::entity>& retron::position::velocity_changed_sink()
{
    return this->velocity_changed_signal;
}

ff::signal_sink<entt::entity>& retron::position::direction_changed_sink()
{
    return this->direction_changed_signal;
}

ff::signal_sink<entt::entity>& retron::position::scale_changed_sink()
{
    return this->scale_changed_signal;
}

ff::signal_sink<entt::entity>& retron::position::rotation_changed_sink()
{
    return this->rotation_changed_signal;
}

void retron::position::render_debug(ff::draw_base& draw)
{
    for (auto [entity, pc] : this->registry.view<::position_component>().each())
    {
        draw.draw_palette_filled_rectangle(ff::rect_fixed(pc.position + ff::point_fixed(-1, -1), pc.position + ff::point_fixed(1, 1)), 230);
    }
}

void retron::position::position_changed(entt::registry& registry, entt::entity entity)
{
    this->position_changed_signal.notify(entity);
}

void retron::position::velocity_changed(entt::registry& registry, entt::entity entity)
{
    this->velocity_changed_signal.notify(entity);
}

void retron::position::direction_changed(entt::registry& registry, entt::entity entity)
{
    this->direction_changed_signal.notify(entity);
}

void retron::position::scale_changed(entt::registry& registry, entt::entity entity)
{
    this->scale_changed_signal.notify(entity);
}

void retron::position::rotation_changed(entt::registry& registry, entt::entity entity)
{
    this->rotation_changed_signal.notify(entity);
}
