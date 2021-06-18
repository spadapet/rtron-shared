#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_spec.h"
#include "source/level/collision.h"
#include "source/level/components.h"
#include "source/level/entities.h"
#include "source/level/level_logic.h"
#include "source/level/position.h"

namespace anim_events
{
    static const size_t NEW_PARTICLES = ff::stable_hash_func("new_particles"sv);
    static const size_t DELETE_ANIMATION = ff::stable_hash_func("delete_animation"sv);
};

static ff::point_fixed get_press_vector(const ff::input_event_provider& input_events, bool for_shoot)
{
    ff::fixed_int joystick_min = retron::app_service::get().game_spec().joystick_min;

    ff::rect_fixed dir_press(
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_LEFT : retron::input_events::ID_LEFT),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_UP : retron::input_events::ID_UP),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_RIGHT : retron::input_events::ID_RIGHT),
        input_events.analog_value(for_shoot ? retron::input_events::ID_SHOOT_DOWN : retron::input_events::ID_DOWN));

    dir_press.left = dir_press.left * ff::fixed_int(dir_press.left >= joystick_min);
    dir_press.top = dir_press.top * ff::fixed_int(dir_press.top >= joystick_min);
    dir_press.right = dir_press.right * ff::fixed_int(dir_press.right >= joystick_min);
    dir_press.bottom = dir_press.bottom * ff::fixed_int(dir_press.bottom >= joystick_min);

    ff::point_fixed dir(dir_press.right - dir_press.left, dir_press.bottom - dir_press.top);
    if (dir)
    {
        int slice = retron::helpers::dir_to_degrees(dir) * 2 / 45;

        return ff::point_fixed(
            (slice >= 6 && slice <= 11) ? -1 : ((slice <= 3 || slice >= 14) ? 1 : 0),
            (slice >= 2 && slice <= 7) ? -1 : ((slice >= 10 && slice <= 15) ? 1 : 0));
    }

    return ff::point_fixed(0, 0);
}

retron::level_logic::level_logic()
{}

void retron::level_logic::advance_time(retron::entity_category categories, entt::registry& registry, const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    if (ff::flags::has(categories, retron::entity_category::animation))
    {
        for (auto [entity, comp, pos] : registry.view<retron::comp::animation, const retron::comp::position>().each())
        {
            this->advance_animation(registry, entity, comp, pos);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::enemy))
    {
        for (auto [entity, comp, pos] : registry.view<retron::comp::grunt, const retron::comp::position>().each())
        {
            this->advance_grunt(registry, entity, comp, pos, difficulty_spec, frame_count);
        }

        for (auto [entity, comp, pos, vel] : registry.view<retron::comp::hulk, const retron::comp::position, const retron::comp::velocity>().each())
        {
            this->advance_hulk(registry, entity, comp, pos, vel, difficulty_spec, frame_count);
        }
    }

    if (ff::flags::has(categories, retron::entity_category::bonus))
    {
        for (auto [entity, comp, pos, vel] : registry.view<retron::comp::bonus, const retron::comp::position, const retron::comp::velocity>().each())
        {
            this->advance_bonus(registry, entity, comp, pos, vel, difficulty_spec, frame_count);
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
            this->advance_player(registry, entity, comp, pos, vel, difficulty_spec, frame_count);
        }
    }
}

void retron::level_logic::advance_player(entt::registry& registry, entt::entity entity, retron::comp::player& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    comp.state_counter++;

    switch (comp.state)
    {
        case retron::comp::player::player_state::ghost:
            if (comp.state_counter >= difficulty_spec.player_ghost_counter)
            {
                comp.state = retron::comp::player::player_state::alive;
            }
            [[fallthrough]];

        case retron::comp::player::player_state::alive:
            {
                ff::point_fixed move_vector = ::get_press_vector(comp.input_events, false);
                ff::point_fixed shot_vector = ::get_press_vector(comp.input_events, true);

                registry.replace<retron::comp::velocity>(entity, move_vector * difficulty_spec.player_move);
                registry.replace<retron::comp::position>(entity, pos.position + vel.velocity);
                registry.replace<retron::comp::direction>(entity, ff::point_fixed(
                    -ff::fixed_int(move_vector.x < 0_f) + ff::fixed_int(move_vector.x > 0_f),
                    -ff::fixed_int(move_vector.y < 0_f) + ff::fixed_int(move_vector.y > 0_f)));

                if (comp.allow_shot_frame <= frame_count && shot_vector)
                {
                    comp.state = retron::comp::player::player_state::alive; // stop being an invincible ghost after shooting
                    comp.allow_shot_frame = frame_count + difficulty_spec.player_shot_counter;
                    this->create_player_bullet(entity, this->bounds_box(entity).center(), retron::position::canon_direction(shot_vector));
                }
            }
            break;

        case retron::comp::player::player_state::dead:
            if (comp.state_counter >= difficulty_spec.player_dead_counter && this->game_service.coop_take_life(comp.player.get()))
            {
                this->entities.delay_delete(entity);
                this->create_player(comp.player.get());
            }
            break;
    }
}

void retron::level_logic::advance_grunt(entt::registry& registry, entt::entity entity, retron::comp::grunt& comp, const retron::comp::position& pos, const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    if (!comp.move_frame)
    {
        comp.move_frame = this->pick_grunt_move_frame(difficulty_spec, frame_count);
    }
    else if (comp.move_frame <= frame_count)
    {
        comp.move_frame = this->pick_grunt_move_frame(difficulty_spec, frame_count);

        entt::entity dest_entity = this->player_target(comp.index);
        if (dest_entity != entt::null)
        {
            comp.dest_pos = this->pick_move_destination(entity, dest_entity, retron::collision_box_type::grunt_avoid_box);
        }

        ff::point_fixed delta = comp.dest_pos - pos.position;
        ff::point_fixed vel(
            std::copysign(difficulty_spec.grunt_move.x, delta.x ? delta.x : (ff::math::random_bool() ? 1 : -1)),
            std::copysign(difficulty_spec.grunt_move.y, delta.y ? delta.y : (ff::math::random_bool() ? 1 : -1)));

        registry.replace<retron::comp::position>(entity, pos.position + vel);
    }
}

void retron::level_logic::advance_hulk(entt::registry& registry, entt::entity entity, retron::comp::hulk& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    if (comp.force_turn || !vel.velocity || frame_count >= this->next_hulk_group_turn[comp.group] || !registry.valid(comp.target_entity))
    {
        if (!registry.valid(comp.target_entity))
        {
            comp.target_entity = this->pick_hulk_target(entity);
        }

        if (registry.valid(comp.target_entity))
        {
            const retron::difficulty_spec& diff = difficulty_spec;

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

    if (comp.force_push || !(frame_count % 8))
    {
        registry.replace<retron::comp::position>(entity, pos.position + vel.velocity + comp.force_push);
        comp.force_push = {};
    }
}

void retron::level_logic::advance_bonus(entt::registry& registry, entt::entity entity, retron::comp::bonus& comp, const retron::comp::position& pos, const retron::comp::velocity& vel, const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    if (comp.turn_frame <= frame_count || !vel.velocity)
    {
        registry.replace<retron::comp::velocity>(entity, retron::helpers::index_to_dir(ff::math::random_range(0, 7)) * difficulty_spec.bonus_move);
        comp.turn_frame = frame_count + ff::math::random_range(difficulty_spec.bonus_min_ticks, difficulty_spec.bonus_max_ticks) * difficulty_spec.bonus_tick_frames;
    }

    if (!((frame_count - comp.turn_frame) % difficulty_spec.bonus_tick_frames))
    {
        registry.replace<retron::comp::position>(entity, pos.position + vel.velocity);
    }
}

void retron::level_logic::advance_animation(entt::registry& registry, entt::entity entity, retron::comp::animation& comp, const retron::comp::position& pos)
{
    std::forward_list<ff::animation_event> events;
    comp.anim->advance_animation(&ff::push_front_collection(events));

    for (const ff::animation_event& event : events)
    {
        if (event.event_id == anim_events::NEW_PARTICLES && event.params)
        {
            std::string name = event.params->get<std::string>("name");
            auto i = this->particle_effects.find(name);
            assert(i != this->particle_effects.end());

            if (i != this->particle_effects.end())
            {
                i->second.add(this->particles, pos.position);
            }
        }
        else if (event.event_id == anim_events::DELETE_ANIMATION)
        {
            registry.emplace_or_replace<retron::comp::flag::pending_delete>(entity);
        }
    }
}

size_t retron::level_logic::pick_grunt_move_frame(const retron::difficulty_spec& difficulty_spec, size_t frame_count)
{
    size_t i = std::min<size_t>(frame_count / difficulty_spec.grunt_max_ticks_rate, difficulty_spec.grunt_max_ticks - 1);
    i = ff::math::random_range(1u, difficulty_spec.grunt_max_ticks - i) * difficulty_spec.grunt_tick_frames;
    return frame_count + std::max<size_t>(i, difficulty_spec.grunt_min_ticks);
}
