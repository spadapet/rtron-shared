#include "pch.h"
#include "source/core/game_spec.h"
#include "source/level/components.h"
#include "source/level/entities.h"
#include "source/level/position.h"

ff::rect_fixed retron::entity_util::hit_box_spec(retron::entity_type type)
{
    switch (retron::entity_util::category(type))
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

ff::rect_fixed retron::entity_util::bounds_box_spec(retron::entity_type type)
{
    switch (retron::entity_util::category(type))
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

    return retron::entity_util::hit_box_spec(type);
}

bool retron::entity_util::can_hit_box_collide(retron::entity_type type_a, retron::entity_type type_b)
{
    if (type_a > type_b)
    {
        std::swap(type_a, type_b);
    }

    switch (retron::entity_util::category(type_a))
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

bool retron::entity_util::can_bounds_box_collide(retron::entity_type type_a, retron::entity_type type_b)
{
    // One of the two needs to be a level box
    return ff::flags::has(ff::flags::toggle(type_a, type_b), retron::entity_type::category_level);
}

size_t retron::entity_util::index(retron::entity_type type)
{
    switch (type)
    {
        default:
            return 0;

        case retron::entity_type::bullet_player_1:
        case retron::entity_type::player_1:
        case retron::entity_type::electrode_1:
            return 1;

        case retron::entity_type::electrode_2:
            return 2;

        case retron::entity_type::electrode_3:
            return 3;

        case retron::entity_type::bonus_woman: return static_cast<size_t>(retron::bonus_type::woman);
        case retron::entity_type::bonus_man: return static_cast<size_t>(retron::bonus_type::man);
        case retron::entity_type::bonus_girl: return static_cast<size_t>(retron::bonus_type::girl);
        case retron::entity_type::bonus_boy: return static_cast<size_t>(retron::bonus_type::boy);
        case retron::entity_type::bonus_dog: return static_cast<size_t>(retron::bonus_type::dog);
    }
}

retron::entity_type retron::entity_util::bonus(retron::bonus_type type)
{
    switch (type)
    {
        case retron::bonus_type::woman: return retron::entity_type::bonus_woman;
        case retron::bonus_type::man: return retron::entity_type::bonus_man;
        case retron::bonus_type::girl: return retron::entity_type::bonus_girl;
        case retron::bonus_type::boy: return retron::entity_type::bonus_boy;
        case retron::bonus_type::dog: return retron::entity_type::bonus_dog;
        default: assert(false); return retron::entity_type::none;
    }
}

retron::entity_type retron::entity_util::player(size_t index)
{
    switch (index)
    {
        case 0: return retron::entity_type::player_0;
        case 1: return retron::entity_type::player_1;
        default: assert(false); return retron::entity_type::none;
    }
}

retron::entity_type retron::entity_util::electrode(size_t index)
{
    switch (index)
    {
        case 0: return retron::entity_type::electrode_0;
        case 1: return retron::entity_type::electrode_1;
        case 2: return retron::entity_type::electrode_2;
        case 3: return retron::entity_type::electrode_3;
        default: assert(false); return retron::entity_type::none;
    }
}

retron::entity_type retron::entity_util::bullet_for_player(retron::entity_type type)
{
    return retron::entity_util::index(type) ? retron::entity_type::bullet_player_1 : retron::entity_type::bullet_player_0;
}

std::pair<std::string_view, std::string_view> retron::entity_util::start_particle_names_0_90(retron::entity_type type)
{
    switch (type)
    {
        case retron::entity_type::enemy_grunt:
            return std::make_pair("grunt_start_0"sv, "grunt_start_90"sv);

        case retron::entity_type::enemy_hulk:
            return std::make_pair("hulk_start_0"sv, "hulk_start_90"sv);
    }

    return {};
}

retron::entities::entities(entt::registry& registry)
    : registry(registry)
{
    this->connections.emplace_front(this->registry.on_construct<retron::comp::flag::pending_delete>().connect<&retron::entities::handle_deleting>(this));
    this->connections.emplace_front(this->registry.on_destroy<retron::comp::flag::pending_delete>().connect<&retron::entities::handle_deleted>(this));
}

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
    this->entity_created_signal.notify(entity, type);
    return entity;
}

entt::entity retron::entities::create(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->registry.create();
    this->registry.emplace<retron::entity_type>(entity, type);
    this->registry.emplace<retron::comp::position>(entity, pos);
    this->entity_created_signal.notify(entity, type);
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
    this->registry.emplace<retron::comp::rotation>(entity, retron::helpers::dir_to_degrees(retron::position::canon_direction(vel)));
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
        this->entity_deleted_signal.notify(entity);
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

void retron::entities::handle_deleting(entt::registry& registry, entt::entity entity)
{
    this->entity_deleting_signal.notify(entity);
}

void retron::entities::handle_deleted(entt::registry& registry, entt::entity entity)
{
    // this->entity_deleted_signal.notify(entity);
}
