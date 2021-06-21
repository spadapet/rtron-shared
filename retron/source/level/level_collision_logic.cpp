#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/particles.h"
#include "source/level/collision.h"
#include "source/level/components.h"
#include "source/level/entities.h"
#include "source/level/entity_type.h"
#include "source/level/entity_util.h"
#include "source/level/level_collision_logic.h"

retron::level_collision_logic::level_collision_logic(retron::level_logic_host& host, retron::entities& entities, retron::collision& collision)
    : host(host)
    , entities(entities)
    , collision(collision)
    , bonus_collected(0)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level_collision_logic::init_resources, this)));

    this->init_resources();
}

void retron::level_collision_logic::handle_collisions()
{
    for (auto& [entity1, entity2] : this->collision.detect_collisions(this->collisions, retron::collision_box_type::bounds_box))
    {
        if (!this->entities.deleted(entity1) && !this->entities.deleted(entity2))
        {
            this->handle_bounds_collision(entity1, entity2);
        }
    }

    for (auto& [entity1, entity2] : this->collision.detect_collisions(this->collisions, retron::collision_box_type::hit_box))
    {
        if (!this->entities.deleted(entity1) && !this->entities.deleted(entity2))
        {
            this->handle_entity_collision(entity1, entity2);
            this->handle_entity_collision(entity2, entity1);
        }
    }
}

void retron::level_collision_logic::reset()
{
    this->bonus_collected = 0;
}

void retron::level_collision_logic::init_resources()
{
    this->electrode_die_anims[0] = "anim.electrode_die[0]";
    this->electrode_die_anims[1] = "anim.electrode_die[1]";
    this->electrode_die_anims[2] = "anim.electrode_die[2]";

    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::woman)] = "anim.bonus_die_adult";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::man)] = "anim.bonus_die_adult";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::girl)] = "anim.bonus_die_child";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::boy)] = "anim.bonus_die_child";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::dog)] = "anim.bonus_die_pet";
}

ff::rect_fixed retron::level_collision_logic::bounds_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::bounds_box);
}

void retron::level_collision_logic::handle_bounds_collision(entt::entity target_entity, entt::entity level_entity)
{
    entt::registry& registry = this->host.host_registry();

    ff::rect_fixed old_rect = this->bounds_box(target_entity);
    ff::rect_fixed new_rect = (this->entities.type(level_entity) == retron::entity_type::level_bounds)
        ? old_rect.move_inside(this->bounds_box(level_entity))
        : old_rect.move_outside(this->bounds_box(level_entity));

    ff::point_fixed offset = new_rect.top_left() - old_rect.top_left();
    this->entities.position(target_entity, this->entities.position(target_entity) + offset);

    switch (this->entities.category(target_entity))
    {
        case retron::entity_category::bullet:
            this->destroy_bullet(target_entity, level_entity);
            break;

        case retron::entity_category::enemy:
            if (offset && this->entities.type(target_entity) == retron::entity_type::enemy_hulk)
            {
                registry.get<retron::comp::hulk>(target_entity).force_turn = true;
            }
            break;

        case retron::entity_category::bonus:
            if (offset)
            {
                registry.get<retron::comp::bonus>(target_entity).turn_frame = 0;
            }
            break;
    }
}

void retron::level_collision_logic::handle_entity_collision(entt::entity target_entity, entt::entity source_entity)
{
    entt::registry& registry = this->host.host_registry();
    retron::entity_type target_type = this->entities.type(target_entity);
    retron::entity_type source_type = this->entities.type(source_entity);
    retron::entity_category target_category = this->entities.category(target_entity);
    retron::entity_category source_category = this->entities.category(source_entity);

    switch (target_category)
    {
        case retron::entity_category::player:
            switch (source_category)
            {
                case retron::entity_category::enemy:
                case retron::entity_category::electrode:
                    if (!ff::flags::has(retron::app_service::get().debug_cheats(), retron::debug_cheats_t::invincible))
                    {
                        retron::comp::player& data = registry.get<retron::comp::player>(target_entity);
                        if (data.state == retron::comp::player::player_state::alive)
                        {
                            data.state = retron::comp::player::player_state::dead;
                            data.state_counter = 0;
                        }
                    }
                    break;
            }
            break;

        case retron::entity_category::bonus:
            switch (source_category)
            {
                case retron::entity_category::player:
                    this->entities.delay_delete(target_entity);
                    this->add_points(source_entity, target_entity);
                    break;

                case retron::entity_category::enemy:
                    if (source_type == retron::entity_type::enemy_hulk && !registry.all_of<retron::comp::showing_particle_effect>(source_entity))
                    {
                        this->destroy_bonus(target_entity, source_entity);
                    }
                    break;

                case retron::entity_category::electrode:
                    this->handle_bounds_collision(target_entity, source_entity);
                    break;
            }
            break;

        case retron::entity_category::enemy:
            switch (source_category)
            {
                case entity_category::electrode:
                    if (target_type == retron::entity_type::enemy_grunt)
                    {
                        this->destroy_enemy(target_entity, source_entity);
                    }
                    break;

                case entity_category::bullet:
                    if (target_type == retron::entity_type::enemy_hulk)
                    {
                        this->push_hulk(target_entity, source_entity);
                    }
                    else
                    {
                        this->destroy_enemy(target_entity, source_entity);
                        this->add_points(source_entity, target_entity);
                    }
                    break;
            }
            break;

        case retron::entity_category::electrode:
            switch (source_category)
            {
                case retron::entity_category::enemy:
                    this->destroy_electrode(target_entity, source_entity);
                    break;

                case retron::entity_category::bullet:
                    this->destroy_electrode(target_entity, source_entity);
                    this->add_points(source_entity, target_entity);
                    break;
            }
            break;

        case retron::entity_category::bullet:
            switch (source_category)
            {
                case retron::entity_category::electrode:
                    this->destroy_bullet(target_entity, source_entity);
                    break;

                case retron::entity_category::enemy:
                    if (source_type == retron::entity_type::enemy_hulk)
                    {
                        this->handle_bounds_collision(target_entity, source_entity);
                    }
                    else
                    {
                        this->destroy_bullet(target_entity, source_entity);
                    }
                    break;
            }
            break;
    }
}

void retron::level_collision_logic::destroy_bullet(entt::entity bullet_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(bullet_entity))
    {
        retron::entity_category by_category = this->entities.category(by_entity);
        if (by_category != retron::entity_category::enemy)
        {
            ff::point_fixed vel = this->entities.velocity(bullet_entity);
            ff::fixed_int angle = ff::math::radians_to_degrees(std::atan2(-vel));
            ff::point_fixed pos = this->entities.position(bullet_entity);
            ff::point_fixed pos2(pos.x + (vel.x ? std::copysign(1_f, vel.x) : 0), pos.y + (vel.y ? std::copysign(1_f, vel.y) : 0));

            retron::particle_effect_options options;
            options.angle = std::make_pair(angle - 60_f, angle + 60_f);
            options.type = static_cast<uint8_t>(retron::entity_util::index(this->entities.type(bullet_entity)));
            this->host.host_create_particles("player_bullet_explode", pos, &options);

            if (by_category != retron::entity_category::electrode)
            {
                this->host.host_create_particles("player_bullet_smoke", pos2);
            }
        }
        else
        {
            ff::point_fixed pos = this->bounds_box(by_entity).center();
            retron::particle_effect_options options;
            options.type = static_cast<uint8_t>(retron::entity_util::index(this->entities.type(bullet_entity)));
            this->host.host_create_particles("player_bullet_explode", pos, &options);
            this->host.host_create_particles("player_bullet_smoke", pos);
        }
    }
}

void retron::level_collision_logic::destroy_enemy(entt::entity entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(entity))
    {
        auto names = retron::entity_util::start_particle_names_0_90(this->entities.type(entity));
        if (names.first.size())
        {
            retron::particle_effect_options options;
            options.reverse = true;

            std::string_view name;
            ff::point_fixed center = this->bounds_box(entity).center();
            bool by_bullet = this->entities.category(by_entity) == retron::entity_category::bullet;

            switch (retron::helpers::dir_to_index(this->entities.velocity(by_bullet ? by_entity : entity)))
            {
                case 0:
                case 4:
                    name = names.second;
                    break;

                case 1:
                case 5:
                    options.rotate = 45;
                    name = names.first;
                    break;

                case 2:
                case 6:
                    name = names.first;
                    break;

                case 3:
                case 7:
                    options.rotate = -45;
                    name = names.first;
                    break;
            }

            if (name.size())
            {
                this->host.host_create_particles(name, center, &options);
            }
        }
    }
}

void retron::level_collision_logic::destroy_electrode(entt::entity electrode_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(electrode_entity))
    {
        retron::entity_type electrode_type = this->entities.type(electrode_entity);
        std::shared_ptr<ff::animation_base> anim = this->electrode_die_anims[retron::entity_util::index(electrode_type)].object();

        this->entities.create_animation(anim, this->entities.position(electrode_entity), false);
    }
}

void retron::level_collision_logic::destroy_bonus(entt::entity bonus_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(bonus_entity))
    {
        retron::entity_type type = this->entities.type(bonus_entity);
        std::shared_ptr<ff::animation_base> anim = this->bonus_die_anims[retron::entity_util::index(type)].object();
        this->entities.create_animation(anim, this->entities.position(bonus_entity), true);
    }
}

void retron::level_collision_logic::push_hulk(entt::entity enemy_entity, entt::entity by_entity)
{
    entt::registry& registry = this->host.host_registry();
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();

    ff::point_fixed by_vel = this->entities.velocity(by_entity);
    registry.get<retron::comp::hulk>(enemy_entity).force_push = ff::point_fixed(
        diff.hulk_push.x * (by_vel.x == 0_f ? 0_f : (by_vel.x < 0_f ? -1_f : 1_f)),
        diff.hulk_push.y * (by_vel.y == 0_f ? 0_f : (by_vel.y < 0_f ? -1_f : 1_f)));
}

void retron::level_collision_logic::add_points(entt::entity player_or_bullet_entity, entt::entity destroyed_entity)
{
    entt::registry& registry = this->host.host_registry();
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();

    size_t points = 0;
    const retron::player* player = nullptr;

    for (auto [e, p] : registry.view<retron::comp::player>().each())
    {
        if (p.player.get().index == retron::entity_util::index(this->entities.type(player_or_bullet_entity)))
        {
            player = &p.player.get();
        }
    }

    switch (this->entities.category(destroyed_entity))
    {
        case retron::entity_category::enemy:
            switch (this->entities.type(destroyed_entity))
            {
                case retron::entity_type::enemy_grunt:
                    points = diff.points_grunt;
                    break;
            }
            break;

        case retron::entity_category::electrode:
            points = diff.points_electrode;
            break;

        case retron::entity_category::bonus:
            points = diff.bonus_points[this->bonus_collected];
            this->bonus_collected = std::min(this->bonus_collected + 1, diff.bonus_points.size() - 1);
            break;

        default:
            return;
    }

    if (player && points)
    {
        this->host.host_add_points(*player, points);
    }
}
