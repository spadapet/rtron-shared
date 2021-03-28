#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/level_service.h"
#include "source/level/level.h"

static constexpr std::string_view PARTICLES_destroy_grunt = "destroy_grunt"sv;
static constexpr std::string_view PARTICLES_player_bullet_hit_bounds = "player_bullet_hit_bounds"sv;
static constexpr std::string_view PARTICLES_player_enter = "player_enter"sv;

static constexpr std::string_view ALL_PARTICLE_EFFECTS[] =
{
    ::PARTICLES_destroy_grunt,
    ::PARTICLES_player_bullet_hit_bounds,
    ::PARTICLES_player_enter,
};

namespace
{
    struct player_data
    {
        size_t index_in_level;
        size_t shot_counter;
    };

    struct player_bullet_data
    {
        entt::entity player;
        size_t start_frame;
    };

    struct grunt_data
    {
        size_t index;
        size_t move_counter;
        ff::point_fixed dest_pos;
    };

    struct particle_effect_follows_entity
    {
        int effect_id;
        ff::point_fixed offset;
    };
}

retron::level::level(const retron::level_service& level_service)
    : level_service_(level_service)
    , game_spec_(retron::app_service::get().game_spec())
    , level_spec_(level_service.level_spec())
    , difficulty_spec_(level_service.difficulty_spec())
    , frame_count(0)
    , entities(this->registry)
    , position(this->registry)
    , collision(this->registry, this->position, this->entities)
{
    this->init_resources();

    this->connections.emplace_front(retron::app_service::get().destroyed().connect(std::bind(&retron::level::app_service_destroyed, this)));
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level::init_resources, this)));

    for (size_t i = 0; i < this->level_service_.player_count(); i++)
    {
        this->create_player(i);
    }

    std::vector<ff::rect_fixed> avoid_rects;
    avoid_rects.reserve(this->level_spec_.rects.size());

    for (const retron::level_rect& level_rect : this->level_spec_.rects)
    {
        switch (level_rect.type)
        {
            case retron::level_rect::type::bounds:
                this->create_bounds(level_rect.rect.deflate(constants::LEVEL_BORDER_THICKNESS, constants::LEVEL_BORDER_THICKNESS));
                break;

            case retron::level_rect::type::box:
                this->create_box(level_rect.rect);
                avoid_rects.push_back(level_rect.rect);
                break;

            case retron::level_rect::type::safe:
                avoid_rects.push_back(level_rect.rect);
                break;
        }
    }

    for (const retron::level_objects_spec& object_spec : this->level_spec_.objects)
    {
        this->create_objects(object_spec.bonus_woman, entity_type::bonus_woman, object_spec.rect, avoid_rects);
        this->create_objects(object_spec.bonus_man, entity_type::bonus_man, object_spec.rect, avoid_rects);
        this->create_objects(object_spec.bonus_child, entity_type::bonus_child, object_spec.rect, avoid_rects);
        this->create_objects(object_spec.electrode, entity_type::electrode, object_spec.rect, avoid_rects);
        this->create_objects(object_spec.hulk, entity_type::hulk, object_spec.rect, avoid_rects);
        this->create_objects(object_spec.grunt, entity_type::grunt, object_spec.rect, avoid_rects);
    }
}

void retron::level::advance(const ff::rect_fixed& camera_rect)
{
    this->frame_count++;

    ff::end_scope_action particle_scope = this->particles.advance_async();
    this->enum_entities(std::bind(&retron::level::advance_entity, this, std::placeholders::_1, std::placeholders::_2));
    this->handle_collisions();
    this->advance_particle_effect_positions();
    this->entities.flush_delete();
}

void retron::level::render(ff::dx11_target_base& target, ff::dx11_depth& depth, const ff::rect_fixed& target_rect, const ff::rect_fixed& camera_rect)
{
    ff::draw_ptr draw = retron::app_service::get().draw_device().begin_draw(target, &depth, target_rect, camera_rect);
    if (draw)
    {
        this->enum_entities(std::bind(&retron::level::render_entity, this, std::placeholders::_1, std::placeholders::_2, std::ref(*draw)));
        this->render_particles(*draw);
        this->render_debug(*draw);
    }
}

void retron::level::app_service_destroyed()
{
    this->connections.clear();
}

void retron::level::init_resources()
{
    this->player_walk_anims[0] = "anim.player_walk_right"sv;
    this->player_walk_anims[1] = "anim.player_walk_right_up"sv;
    this->player_walk_anims[2] = "anim.player_walk_up"sv;
    this->player_walk_anims[3] = "anim.player_walk_left_up"sv;
    this->player_walk_anims[4] = "anim.player_walk_left"sv;
    this->player_walk_anims[5] = "anim.player_walk_left_down"sv;
    this->player_walk_anims[6] = "anim.player_walk_down"sv;
    this->player_walk_anims[7] = "anim.player_walk_right_down"sv;

    this->player_bullet_anim = "sprites.player_bullet"sv;

    this->grunt_walk_anim = "sprites.grunt"sv;

    ff::dict level_particles_dict = ff::auto_resource_value("level_particles"sv).value()->get<ff::dict>();
    for (std::string_view name : ::ALL_PARTICLE_EFFECTS)
    {
        this->particle_effects.try_emplace(name, retron::particles::effect_t(level_particles_dict.get(name)));
    }
}

entt::entity retron::level::create_entity(entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->entities.create(type);
    this->position.set(entity, pos);

    switch (type)
    {
        case entity_type::grunt:
            this->registry.emplace<::grunt_data>(entity, this->registry.size<::grunt_data>(), this->pick_grunt_move_counter(), pos);
            break;
    }

    return entity;
}

entt::entity retron::level::create_player(size_t index_in_level)
{
    ff::point_fixed pos = this->level_spec_.player_start;
    pos.x += index_in_level * 16 - this->level_service_.player_count() * 8 + 8;

    entt::entity entity = this->create_entity(entity_type::player, pos);
    this->registry.emplace<::player_data>(entity, ::player_data{ index_in_level, 0 });

    retron::player& player = this->level_service_.player(index_in_level);
    retron::particles::effect_options options;
    options.type = static_cast<uint8_t>(player.index);

    int effect_id = this->particle_effects[::PARTICLES_player_enter].add(this->particles, pos, &options);
    this->registry.emplace<::particle_effect_follows_entity>(entity, ::particle_effect_follows_entity{ effect_id, ff::point_fixed(0, 0) });

    return entity;
}

entt::entity retron::level::create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir)
{
    ff::point_fixed vel(this->difficulty_spec_.player_shot_move * shot_dir.x, this->difficulty_spec_.player_shot_move * shot_dir.y);
    entt::entity entity = this->create_entity(entity_type::player_bullet, shot_pos + vel);
    this->registry.emplace<::player_bullet_data>(entity, player, this->frame_count);

    this->position.velocity(entity, vel);
    this->position.rotation(entity, helpers::dir_to_degrees(shot_dir));

    return entity;
}

entt::entity retron::level::create_bounds(const ff::rect_fixed& rect)
{
    entt::entity entity = this->entities.create(entity_type::level_bounds);
    this->collision.box(entity, rect, entity_box_type::level, retron::collision_box_type::bounds_box);
    return entity;
}

entt::entity retron::level::create_box(const ff::rect_fixed& rect)
{
    entt::entity entity = this->entities.create(entity_type::level_box);
    this->collision.box(entity, rect, entity_box_type::level, retron::collision_box_type::bounds_box);
    return entity;
}

void retron::level::create_objects(size_t count, entity_type type, const ff::rect_fixed& rect, const std::vector<ff::rect_fixed>& avoid_rects)
{
    const ff::rect_fixed& hit_spec = retron::get_hit_box_spec(type);

    for (size_t i = 0; i < count; i++)
    {
        for (size_t attempt = 0; attempt < 2048; attempt++)
        {
            ff::point_fixed pos(
                (ff::math::random_non_negative() & ~0x3) % static_cast<int>(rect.width() - hit_spec.width()) - static_cast<int>(hit_spec.left) + static_cast<int>(rect.left),
                (ff::math::random_non_negative() & ~0x3) % static_cast<int>(rect.height() - hit_spec.height()) - static_cast<int>(hit_spec.top) + static_cast<int>(rect.top));

            ff::rect_fixed hit_rect = hit_spec + pos;
            bool good_pos = true;

            for (const ff::rect_fixed& avoid_rect : avoid_rects)
            {
                if (hit_rect.intersects(avoid_rect))
                {
                    good_pos = false;
                    break;
                }
            }

            if (good_pos)
            {
                this->create_entity(type, pos);
                break;
            }
        }
    }
}

ff::rect_fixed retron::level::bounds_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::bounds_box);
}

ff::rect_fixed retron::level::hit_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::hit_box);
}

void retron::level::advance_entity(entt::entity entity, entity_type type)
{
    switch (type)
    {
        case entity_type::player:
            this->advance_player(entity);
            break;

        case entity_type::player_bullet:
            this->advance_player_bullet(entity);
            break;

        case entity_type::grunt:
            this->advance_grunt(entity);
            break;
    }
}

void retron::level::advance_player(entt::entity entity)
{
    ::player_data& player_data = this->registry.get<::player_data>(entity);
    retron::player& player = this->level_service_.player(player_data.index_in_level);
    const ff::input_event_provider& input_events = this->level_service_.input_events(player);

    ff::rect_fixed dir_press(
        input_events.analog_value(input_events::ID_LEFT),
        input_events.analog_value(input_events::ID_UP),
        input_events.analog_value(input_events::ID_RIGHT),
        input_events.analog_value(input_events::ID_DOWN));

    dir_press.left = (dir_press.left >= this->game_spec_.joystick_min) ? dir_press.left * this->difficulty_spec_.player_move : 0;
    dir_press.top = (dir_press.top >= this->game_spec_.joystick_min) ? dir_press.top * this->difficulty_spec_.player_move : 0;
    dir_press.right = (dir_press.right >= this->game_spec_.joystick_min) ? dir_press.right * this->difficulty_spec_.player_move : 0;
    dir_press.bottom = (dir_press.bottom >= this->game_spec_.joystick_min) ? dir_press.bottom * this->difficulty_spec_.player_move : 0;

    ff::point_fixed dir = this->position.direction(entity);
    dir.x = dir_press.left ? -1 : (dir_press.right ? 1 : 0);
    dir.y = dir_press.top ? -1 : (dir_press.bottom ? 1 : 0);

    ff::point_fixed pos = this->position.get(entity);
    ff::point_fixed vel(dir_press.right - dir_press.left, dir_press.bottom - dir_press.top);
    pos += vel;

    this->position.set(entity, pos);
    this->position.velocity(entity, vel);

    if (dir)
    {
        this->position.direction(entity, dir);
    }

    if (player_data.shot_counter)
    {
        player_data.shot_counter--;
    }

    if (!player_data.shot_counter)
    {
        ff::rect_int shot_press(
            input_events.digital_value(input_events::ID_SHOOT_LEFT),
            input_events.digital_value(input_events::ID_SHOOT_UP),
            input_events.digital_value(input_events::ID_SHOOT_RIGHT),
            input_events.digital_value(input_events::ID_SHOOT_DOWN));

        if (shot_press)
        {
            player_data.shot_counter = this->difficulty_spec_.player_shot_counter;

            ff::point_fixed shot_pos = this->bounds_box(entity).center();
            ff::point_fixed shot_dir(
                shot_press.left ? -1 : (shot_press.right ? 1 : 0),
                shot_press.top ? -1 : (shot_press.bottom ? 1 : 0));

            this->create_player_bullet(entity, shot_pos, shot_dir);
        }
    }
}

void retron::level::advance_player_bullet(entt::entity entity)
{
    ff::point_fixed pos = this->position.get(entity);
    ff::point_fixed vel = this->position.velocity(entity);
    this->position.set(entity, pos + vel);
}

void retron::level::advance_grunt(entt::entity entity)
{
    ::grunt_data& data = this->registry.get<::grunt_data>(entity);
    if (!--data.move_counter)
    {
        data.move_counter = this->pick_grunt_move_counter();

        const size_t players = this->registry.size<::player_data>();
        entt::entity dest_entity = players ? this->registry.view<::player_data>().data()[data.index % players] : entity;
        ff::point_fixed grunt_pos = this->position.get(entity);

        data.dest_pos = this->pick_move_destination(entity, dest_entity, retron::collision_box_type::grunt_avoid_box);

        ff::point_fixed delta = data.dest_pos - grunt_pos;
        ff::point_fixed vel(
            std::copysign(this->difficulty_spec_.grunt_move, delta.x ? delta.x : (ff::math::random_bool() ? 1 : -1)),
            std::copysign(this->difficulty_spec_.grunt_move, delta.y ? delta.y : (ff::math::random_bool() ? 1 : -1)));

        this->position.set(entity, grunt_pos + vel);
    }
}

void retron::level::advance_particle_effect_positions()
{
    for (auto [entity, data] : this->registry.view<::particle_effect_follows_entity>().each())
    {
        if (this->particles.effect_active(data.effect_id))
        {
            this->particles.effect_position(data.effect_id, this->bounds_box(entity).center() + data.offset);
        }
        else
        {
            this->registry.remove<::particle_effect_follows_entity>(entity);
        }
    }
}

void retron::level::handle_collisions()
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
        }
    }
}

void retron::level::handle_bounds_collision(entt::entity entity1, entt::entity entity2)
{
    ff::rect_fixed old_rect = this->bounds_box(entity1);
    ff::rect_fixed new_rect = old_rect;

    ff::rect_fixed level_rect = this->bounds_box(entity2);
    entity_type level_type = this->entities.entity_type(entity2);
    switch (level_type)
    {
        case entity_type::level_bounds:
            new_rect = old_rect.move_inside(level_rect);
            break;

        case entity_type::level_box:
            new_rect = old_rect.move_outside(level_rect);
            break;
    }

    ff::point_fixed offset = new_rect.top_left() - old_rect.top_left();
    ff::point_fixed pos = this->position.get(entity1) + offset;
    this->position.set(entity1, pos);

    switch (this->entities.entity_type(entity1))
    {
        case entity_type::player_bullet:
            this->handle_player_bullet_bounds_collision(entity1);
            break;
    }
}

void retron::level::handle_player_bullet_bounds_collision(entt::entity entity)
{
    this->entities.delay_delete(entity);

    ff::point_fixed vel = this->position.velocity(entity);
    ff::fixed_int angle = this->position.reverse_velocity_as_angle(entity);
    ff::point_fixed pos = this->position.get(entity);
    ff::point_fixed pos2(pos.x + (vel.x ? std::copysign(1_f, vel.x) : 0), pos.y + (vel.y ? std::copysign(1_f, vel.y) : 0));
    std::array<ff::point_fixed, 2> pos_array{ pos, pos2 };

    retron::particles::effect_options options;
    options.angle = std::make_pair(angle - 60_f, angle + 60_f);

    // Set palette for bullet particles
    {
        ::player_bullet_data& bullet_data = this->registry.get<::player_bullet_data>(entity);
        ::player_data* player_data = this->registry.valid(bullet_data.player) ? this->registry.try_get<::player_data>(bullet_data.player) : nullptr;
        if (player_data)
        {
            retron::player& player = this->level_service_.player(player_data->index_in_level);
            options.type = static_cast<uint8_t>(player.index);
        }
    }

    this->particle_effects[::PARTICLES_player_bullet_hit_bounds].add(this->particles, pos_array.data(), pos_array.size(), &options);
}

void retron::level::handle_entity_collision(entt::entity entity1, entt::entity entity2)
{
    entity_box_type type1 = this->collision.box_type(entity1, retron::collision_box_type::hit_box);
    entity_box_type type2 = this->collision.box_type(entity2, retron::collision_box_type::hit_box);

    switch (type1)
    {
        case entity_box_type::enemy:
            switch (type2)
            {
                case entity_box_type::player_bullet:
                    switch (this->entities.entity_type(entity1))
                    {
                        case entity_type::grunt:
                            {
                                retron::particles::effect_options options;
                                options.rotate = this->position.velocity_as_angle(entity2);

                                this->particle_effects[::PARTICLES_destroy_grunt].add(
                                    this->particles, this->bounds_box(entity1).center(), &options);
                            }
                            break;
                    }

                    if (this->entities.delay_delete(entity2))
                    {
                        this->entities.delay_delete(entity1);
                    }
                    break;
            }
            break;

        case entity_box_type::player_bullet:
            switch (type2)
            {
                case entity_box_type::enemy_bullet:
                    if (this->entities.delay_delete(entity1))
                    {
                        this->entities.delay_delete(entity2);
                    }
                    break;
            }
            break;
    }
}

void retron::level::render_particles(ff::draw_base& draw)
{
    // Render particles for the default palette, which includes player 0
    this->particles.render(draw);

    // Render particles for other players
    for (size_t i = 0; i < this->level_service_.player_count(); i++)
    {
        retron::player& player = this->level_service_.player(i);
        if (player.index)
        {
            ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
            draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            this->particles.render(draw, static_cast<uint8_t>(player.index));

            draw.pop_palette_remap();
        }
    }
}

void retron::level::render_entity(entt::entity entity, entity_type type, ff::draw_base& draw)
{
    switch (type)
    {
        case entity_type::player:
            this->render_player(entity, draw);
            break;

        case entity_type::player_bullet:
            this->render_player_bullet(entity, draw);
            break;

        case entity_type::bonus_woman:
        case entity_type::bonus_man:
        case entity_type::bonus_child:
            this->render_bonus(entity, type, draw);
            break;

        case entity_type::electrode:
            this->render_electrode(entity, draw);
            break;

        case entity_type::hulk:
            this->render_hulk(entity, draw);
            break;

        case entity_type::grunt:
            this->render_grunt(entity, draw);
            break;

        case entity_type::level_bounds:
        case entity_type::level_box:
            draw.draw_palette_outline_rectangle(this->bounds_box(entity), colors::LEVEL_BORDER,
                (type == entity_type::level_bounds) ? -constants::LEVEL_BORDER_THICKNESS : constants::LEVEL_BOX_THICKNESS);
            break;
    }
}

void retron::level::render_player(entt::entity entity, ff::draw_base& draw)
{
    // Don't render the player while particles are flying together
    if (this->registry.try_get<::particle_effect_follows_entity>(entity))
    {
        return;
    }

    ::player_data& player_data = this->registry.get<::player_data>(entity);
    retron::player& player = this->level_service_.player(player_data.index_in_level);

    ff::palette_base& palette = retron::app_service::get().player_palette(player.index);
    draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

    ff::point_fixed dir = this->position.direction(entity);
    ff::fixed_int frame = this->position.velocity(entity) ? ff::fixed_int(this->frame_count) / this->difficulty_spec_.player_move_frame_divisor : 0_f;
    ff::animation_base* anim = this->player_walk_anims[helpers::dir_to_index(dir)].object().get();
    this->render_animation(entity, draw, anim, frame);

    draw.pop_palette_remap();
}

void retron::level::render_player_bullet(entt::entity entity, ff::draw_base& draw)
{
    ::player_bullet_data& data = this->registry.get<::player_bullet_data>(entity);
    this->render_animation(entity, draw, this->player_bullet_anim.object().get(), this->frame_count - data.start_frame);
}

void retron::level::render_bonus(entt::entity entity, entity_type type, ff::draw_base& draw)
{
    int color = 251;
    switch (type)
    {
        case entity_type::bonus_woman: color = 238; break;
        case entity_type::bonus_man: color = 246; break;
    }

    ff::point_fixed pos = this->position.get(entity);
    draw.draw_palette_filled_rectangle(this->hit_box(entity), color);
}

void retron::level::render_electrode(entt::entity entity, ff::draw_base& draw)
{
    ff::point_fixed pos = this->position.get(entity);
    draw.draw_palette_filled_rectangle(this->hit_box(entity), 45);
}

void retron::level::render_hulk(entt::entity entity, ff::draw_base& draw)
{
    ff::point_fixed pos = this->position.get(entity);
    draw.draw_palette_filled_rectangle(this->hit_box(entity), 235);
}

void retron::level::render_grunt(entt::entity entity, ff::draw_base& draw)
{
    ff::point_fixed pos = this->position.get(entity);
    this->render_animation(entity, draw, this->grunt_walk_anim.object().get(), 0);
}

void retron::level::render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_base* anim, ff::fixed_int frame)
{
    ff::point_float pos = std::floor(this->position.get(entity)).cast<float>();
    ff::point_float scale = this->position.scale(entity).cast<float>();
    ff::fixed_int rotation = this->position.rotation(entity);

    anim->draw_frame(draw, ff::transform(pos, scale, rotation), frame);
}

void retron::level::render_debug(ff::draw_base& draw)
{
    if (retron::app_service::get().render_debug())
    {
        for (auto [entity, data] : this->registry.view<::grunt_data>().each())
        {
            draw.draw_palette_line(this->position.get(entity), data.dest_pos, 245, 1);
        }

        this->collision.render_debug(draw);
        this->position.render_debug(draw);
    }
}

size_t retron::level::pick_grunt_move_counter()
{
    size_t i = std::min<size_t>(this->frame_count / this->difficulty_spec_.grunt_max_ticks_rate, this->difficulty_spec_.grunt_max_ticks - 1);
    i = ff::math::random_range(1u, this->difficulty_spec_.grunt_max_ticks - i) * this->difficulty_spec_.grunt_tick_frames;
    return std::max<size_t>(i, this->difficulty_spec_.grunt_min_ticks);
}

ff::point_fixed retron::level::pick_move_destination(entt::entity entity, entt::entity dest_entity, retron::collision_box_type collision_type)
{
    ff::point_fixed entity_pos = this->position.get(entity);
    ff::point_fixed dest_pos = this->position.get(dest_entity);
    ff::point_fixed result = dest_pos;

    auto [box_entity, box_hit_pos, box_hit_normal] = this->collision.ray_test(entity_pos, dest_pos, entity_box_type::level, collision_type);
    if (box_entity != entt::null && box_hit_pos != dest_pos)
    {
        ff::rect_fixed box = this->collision.box(box_entity, collision_type);
        ff::fixed_int best_dist = -1;
        for (ff::point_fixed corner : box.corners())
        {
            ff::fixed_int dist = (corner - dest_pos).length_squared();
            if (best_dist < 0_f || dist < best_dist)
            {
                // Must be a clear path from the entity to the corner it chooses to move to
                auto [e2, p2, n2] = this->collision.ray_test(entity_pos, corner, entity_box_type::level, collision_type);
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

void retron::level::enum_entities(const std::function<void(entt::entity, retron::entity_type)>& func)
{
    for (size_t i = this->entities.sort_entities(); i != 0; i--)
    {
        entt::entity entity = this->entities.entity(i - 1);
        func(entity, this->entities.entity_type(entity));
    }
}
