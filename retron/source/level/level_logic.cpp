#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/level/collision.h"
#include "source/level/components.h"
#include "source/level/entity_type.h"
#include "source/level/entities.h"
#include "source/level/level_logic.h"
#include "source/level/position.h"

namespace anim_events
{
    static const size_t NEW_PARTICLES = ff::stable_hash_func("new_particles"sv);
    static const size_t DELETE_ANIMATION = ff::stable_hash_func("delete_animation"sv);
};

retron::level_logic::level_logic(level_logic_host& host, retron::collision& collision)
    : host(host)
    , collision(collision)
{}

void retron::level_logic::advance_time(retron::entity_category categories)
{
    entt::registry& registry = this->host.host_registry();
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    size_t frame_count = this->host.host_frame_count();

    if (ff::flags::has(categories, retron::entity_category::animation))
    {
        for (auto [entity, comp, pos] : registry.view<retron::comp::animation, const retron::comp::position>().each())
        {
            this->advance_animation(entity, comp, pos);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::enemy))
    {
        for (auto [entity, comp, pos] : registry.view<retron::comp::grunt, const retron::comp::position>().each())
        {
            this->advance_grunt(entity, comp, pos);
        }

        for (auto [entity, comp, pos, vel] : registry.view<retron::comp::hulk, const retron::comp::position, const retron::comp::velocity>().each())
        {
            this->advance_hulk(entity, comp, pos, vel);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::bonus))
    {
        for (auto [entity, comp, pos, vel] : registry.view<retron::comp::bonus, const retron::comp::position, const retron::comp::velocity>().each())
        {
            this->advance_bonus(entity, comp, pos, vel);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::bullet))
    {
        for (auto [entity, pos, vel] : registry.view<retron::comp::bullet, const retron::comp::position, const retron::comp::velocity>().each())
        {
            registry.replace<retron::comp::position>(entity, pos.position + vel.velocity);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::player))
    {
        for (auto [entity, comp, pos, vel] : registry.view<retron::comp::player, const retron::comp::position, const retron::comp::velocity>().each())
        {
            this->advance_player(entity, comp, pos, vel);
        }
    }

    for (size_t& i : this->next_hulk_group_turn)
    {
        if (i < frame_count)
        {
            i = frame_count + ff::math::random_range(diff.hulk_min_ticks, diff.hulk_max_ticks) * diff.hulk_tick_frames;
        }
    }
}

void retron::level_logic::reset()
{
    this->next_hulk_group_turn.clear();
}

void retron::level_logic::advance_player(entt::entity entity, retron::comp::player& comp, const retron::comp::position& pos, const retron::comp::velocity& vel)
{
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    entt::registry& registry = this->host.host_registry();
    size_t frame_count = this->host.host_frame_count();

    comp.state_counter++;

    switch (comp.state)
    {
        case retron::comp::player::player_state::ghost:
            if (comp.state_counter >= diff.player_ghost_counter)
            {
                comp.state = retron::comp::player::player_state::alive;
            }
            [[fallthrough]];

        case retron::comp::player::player_state::alive:
            {
                ff::point_fixed move_vector = retron::helpers::get_press_vector(comp.input_events, false);
                ff::point_fixed shot_vector = retron::helpers::get_press_vector(comp.input_events, true);

                registry.replace<retron::comp::velocity>(entity, move_vector * diff.player_move);
                registry.replace<retron::comp::position>(entity, pos.position + vel.velocity);
                registry.replace<retron::comp::direction>(entity, ff::point_fixed(
                    -ff::fixed_int(move_vector.x < 0_f) + ff::fixed_int(move_vector.x > 0_f),
                    -ff::fixed_int(move_vector.y < 0_f) + ff::fixed_int(move_vector.y > 0_f)));

                if (comp.allow_shot_frame <= frame_count && shot_vector)
                {
                    comp.state = retron::comp::player::player_state::alive; // stop being an invincible ghost after shooting
                    comp.allow_shot_frame = frame_count + diff.player_shot_counter;
                    this->host.host_create_bullet(entity, shot_vector);
                }
            }
            break;

        case retron::comp::player::player_state::dead:
            if (comp.state_counter >= diff.player_dead_counter)
            {
                this->host.host_handle_dead_player(entity, comp.player.get());
            }
            break;
    }
}

void retron::level_logic::advance_grunt(entt::entity entity, retron::comp::grunt& comp, const retron::comp::position& pos)
{
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    entt::registry& registry = this->host.host_registry();
    size_t frame_count = this->host.host_frame_count();

    if (!comp.move_frame)
    {
        comp.move_frame = this->pick_grunt_move_frame();
    }
    else if (comp.move_frame <= frame_count)
    {
        comp.move_frame = this->pick_grunt_move_frame();

        entt::entity dest_entity = this->pick_grunt_player_target(comp.index);
        if (dest_entity != entt::null)
        {
            comp.dest_pos = this->pick_grunt_move_destination(entity, dest_entity);
        }

        ff::point_fixed delta = comp.dest_pos - pos.position;
        ff::point_fixed vel(
            std::copysign(diff.grunt_move.x, delta.x ? delta.x : (ff::math::random_bool() ? 1 : -1)),
            std::copysign(diff.grunt_move.y, delta.y ? delta.y : (ff::math::random_bool() ? 1 : -1)));

        registry.replace<retron::comp::position>(entity, pos.position + vel);
    }
}

void retron::level_logic::advance_hulk(entt::entity entity, retron::comp::hulk& comp, const retron::comp::position& pos, const retron::comp::velocity& vel)
{
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    entt::registry& registry = this->host.host_registry();
    size_t frame_count = this->host.host_frame_count();

    if (comp.group >= this->next_hulk_group_turn.size())
    {
        this->next_hulk_group_turn.resize(comp.group + 1, 0);
    }

    if (comp.force_turn || !vel.velocity || frame_count >= this->next_hulk_group_turn[comp.group] || !registry.valid(comp.target_entity))
    {
        if (!registry.valid(comp.target_entity))
        {
            comp.target_entity = this->pick_hulk_target(entity);
        }

        if (registry.valid(comp.target_entity))
        {
            if (!vel.velocity || comp.force_turn || ff::math::random_range(0u, diff.hulk_no_move_chance))
            {
                ff::point_fixed target = registry.get<retron::comp::position>(comp.target_entity).position + ff::point_fixed(
                    ff::math::random_range(-diff.hulk_fudge.x, diff.hulk_fudge.x),
                    ff::math::random_range(-diff.hulk_fudge.y, diff.hulk_fudge.y));

                if (vel.velocity.x || (!vel.velocity && ff::math::random_bool()))
                {
                    registry.replace<retron::comp::velocity>(entity, ff::point_fixed(0, diff.hulk_move.y * ((pos.position.y < target.y) ? 1 : -1)));
                }
                else
                {
                    registry.replace<retron::comp::velocity>(entity, ff::point_fixed(diff.hulk_move.x * ((pos.position.x < target.x) ? 1 : -1), 0));
                }
            }
        }

        comp.force_turn = false;
    }

    ff::point_fixed final_vel = comp.force_push + (vel.velocity * ((frame_count % 8) ? 0_f : 1_f));
    comp.force_push = {};

    if (final_vel)
    {
        registry.replace<retron::comp::position>(entity, pos.position + final_vel);
    }
}

void retron::level_logic::advance_bonus(entt::entity entity, retron::comp::bonus& comp, const retron::comp::position& pos, const retron::comp::velocity& vel)
{
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    entt::registry& registry = this->host.host_registry();
    size_t frame_count = this->host.host_frame_count();

    if (comp.turn_frame <= frame_count || !vel.velocity)
    {
        registry.replace<retron::comp::velocity>(entity, retron::helpers::index_to_dir(ff::math::random_range(0, 7)) * diff.bonus_move);
        comp.turn_frame = frame_count + ff::math::random_range(diff.bonus_min_ticks, diff.bonus_max_ticks) * diff.bonus_tick_frames;
    }

    if (!((frame_count - comp.turn_frame) % diff.bonus_tick_frames))
    {
        registry.replace<retron::comp::position>(entity, pos.position + vel.velocity);
    }
}

void retron::level_logic::advance_animation(entt::entity entity, retron::comp::animation& comp, const retron::comp::position& pos)
{
    entt::registry& registry = this->host.host_registry();

    ff::stack_vector<ff::animation_event, 8> events;
    comp.anim->advance_animation(&ff::push_back_collection(events));

    for (const ff::animation_event& event : events)
    {
        if (event.event_id == anim_events::NEW_PARTICLES && event.params)
        {
            this->host.host_create_particles(event.params->get<std::string>("name"), pos.position);
        }
        else if (event.event_id == anim_events::DELETE_ANIMATION)
        {
            registry.emplace_or_replace<retron::comp::flag::pending_delete>(entity);
        }
    }
}

size_t retron::level_logic::pick_grunt_move_frame() const
{
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    size_t frame_count = this->host.host_frame_count();

    size_t i = std::min<size_t>(frame_count / diff.grunt_max_ticks_rate, diff.grunt_max_ticks - 1);
    i = ff::math::random_range(1u, diff.grunt_max_ticks - i) * diff.grunt_tick_frames;
    return frame_count + std::max<size_t>(i, diff.grunt_min_ticks);
}

ff::point_fixed retron::level_logic::pick_grunt_move_destination(entt::entity entity, entt::entity dest_entity) const
{
    const entt::registry& registry = this->host.host_registry();
    ff::point_fixed entity_pos =  registry.get<retron::comp::position>(entity).position;
    ff::point_fixed dest_pos = registry.get<retron::comp::position>(dest_entity).position;
    ff::point_fixed result = dest_pos;

    // Fix the case where the player's foot can get inside of a grunt-avoid box around a level box
    // (since the player's bounding box could be smaller than a grunt)
    {
        ff::stack_vector<entt::entity, 8> box_hits;
        this->collision.hit_test(ff::rect_fixed(dest_pos, dest_pos), ff::push_back_collection(box_hits), retron::entity_category::level, retron::collision_box_type::grunt_avoid_box);

        for (entt::entity box_hit : box_hits)
        {
            if (registry.get<retron::entity_type>(box_hit) == retron::entity_type::level_box)
            {
                dest_pos = this->collision.box(box_hit, retron::collision_box_type::grunt_avoid_box).move_point_outside(dest_pos);
            }
        }
    }

    auto [box_entity, box_hit_pos, box_hit_normal] = this->collision.ray_test(entity_pos, dest_pos, retron::entity_category::level, retron::collision_box_type::grunt_avoid_box);
    if (box_entity != entt::null && box_hit_pos != dest_pos)
    {
        ff::rect_fixed box = this->collision.box(box_entity, retron::collision_box_type::grunt_avoid_box);
        ff::fixed_int best_dist = -1;
        for (ff::point_fixed corner : box.corners())
        {
            ff::fixed_int dist = (corner - dest_pos).length_squared();
            if (best_dist < 0_f || dist < best_dist)
            {
                // Must be a clear path from the entity to the corner it chooses to move to
                auto [e2, p2, n2] = this->collision.ray_test(entity_pos, corner, retron::entity_category::level, retron::collision_box_type::grunt_avoid_box);
                if (e2 == entt::null || p2 == corner)
                {
                    best_dist = dist;
                    result = corner;
                }
            }
        }
    }

    return result;
}

entt::entity retron::level_logic::pick_grunt_player_target(size_t enemy_index) const
{
    const entt::registry& registry = this->host.host_registry();

    auto view = registry.view<const retron::comp::player>();
    for (size_t i = enemy_index; i < enemy_index + view.size(); i++)
    {
        entt::entity entity = view.data()[i % view.size()];
        const retron::comp::player& player_data = registry.get<const retron::comp::player>(entity);

        if (player_data.state == retron::comp::player::player_state::alive)
        {
            return entity;
        }
    }

    return entt::null;
}

entt::entity retron::level_logic::pick_hulk_target(entt::entity entity) const
{
    const entt::registry& registry = this->host.host_registry();
    entt::entity target_entity = entt::null;
    ff::fixed_int target_dist = -1;
    ff::point_fixed pos = registry.get<const retron::comp::position>(entity).position;

    for (entt::entity cur_entity : registry.view<const retron::comp::flag::hulk_target>())
    {
        ff::point_fixed cur_pos = registry.get<const retron::comp::position>(cur_entity).position;
        ff::fixed_int cur_dist = (cur_pos - pos).length_squared();

        if (target_dist < 0_f || cur_dist < target_dist)
        {
            target_entity = cur_entity;
            target_dist = cur_dist;
        }
    }

    return target_entity;
}
