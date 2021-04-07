#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/level_service.h"
#include "source/level/level.h"

namespace
{
    struct player_data
    {
        std::reference_wrapper<const retron::player> player;
        std::reference_wrapper<const retron::player> player_or_coop;
        std::reference_wrapper<const ff::input_event_provider> input_events;
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

    struct electrode_data
    {
        size_t index;
        size_t electrode_type;
    };

    struct particle_effect_follows_entity
    {
        int effect_id;
        ff::point_fixed offset;
    };
}

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

retron::level::level(const retron::level_service& level_service)
    : level_service_(level_service)
    , game_spec_(retron::app_service::get().game_spec())
    , level_spec_(level_service.level_spec())
    , difficulty_spec_(level_service.game_service().difficulty_spec())
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
        this->create_objects(object_spec.bonus_woman, entity_type::bonus_woman, object_spec.rect, avoid_rects, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
        this->create_objects(object_spec.bonus_man, entity_type::bonus_man, object_spec.rect, avoid_rects, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
        this->create_objects(object_spec.bonus_child, entity_type::bonus_child, object_spec.rect, avoid_rects, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
        this->create_objects(object_spec.electrode, entity_type::electrode, object_spec.rect, avoid_rects, std::bind(&retron::level::create_electrode, this, std::placeholders::_1, std::placeholders::_2));
        this->create_objects(object_spec.hulk, entity_type::hulk, object_spec.rect, avoid_rects, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
        this->create_objects(object_spec.grunt, entity_type::grunt, object_spec.rect, avoid_rects, std::bind(&retron::level::create_grunt, this, std::placeholders::_1, std::placeholders::_2));
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
    this->player_walk_anims[0] = "anim.player_walk_right";
    this->player_walk_anims[1] = "anim.player_walk_right_up";
    this->player_walk_anims[2] = "anim.player_walk_up";
    this->player_walk_anims[3] = "anim.player_walk_left_up";
    this->player_walk_anims[4] = "anim.player_walk_left";
    this->player_walk_anims[5] = "anim.player_walk_left_down";
    this->player_walk_anims[6] = "anim.player_walk_down";
    this->player_walk_anims[7] = "anim.player_walk_right_down";

    this->electrode_anims[0] = "sprites.electrode[0]";
    this->electrode_anims[1] = "sprites.electrode[1]";
    this->electrode_anims[2] = "sprites.electrode[2]";

    this->player_bullet_anim = "sprites.player_bullet";

    this->grunt_walk_anim = "sprites.grunt";

    ff::dict level_particles_dict = ff::auto_resource_value("level_particles").value()->get<ff::dict>();
    for (std::string_view name : level_particles_dict.child_names())
    {
        this->particle_effects.try_emplace(name, retron::particles::effect_t(level_particles_dict.get(name)));
    }
}

entt::entity retron::level::create_entity(entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->entities.create(type);
    this->position.set(entity, pos);
    return entity;
}

entt::entity retron::level::create_electrode(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create_entity(type, pos);
    size_t index = this->registry.size<::electrode_data>();
    this->registry.emplace<::electrode_data>(entity, index, this->level_service_.level_index());

    return entity;
}

entt::entity retron::level::create_grunt(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create_entity(type, pos);
    size_t grunt_index = this->registry.size<::grunt_data>();
    this->registry.emplace<::grunt_data>(entity, grunt_index, this->pick_grunt_move_counter(), pos);

    retron::particles::effect_options options;
    options.delay = static_cast<int>(grunt_index);
    options.rotate = ff::math::random_range(1, 10) > 2 ? 90 : 0;
    options.spin = options.rotate;
    int effect_id = this->particle_effects["grunt_start"].add(this->particles, pos, &options);
    this->registry.emplace<::particle_effect_follows_entity>(entity, ::particle_effect_follows_entity{ effect_id, ff::point_fixed(0, 0) });

    return entity;
}

entt::entity retron::level::create_player(size_t index_in_level)
{
    ff::point_fixed pos = this->level_spec_.player_start;
    pos.x += index_in_level * 16 - this->level_service_.player_count() * 8 + 8;

    entt::entity entity = this->create_entity(entity_type::player, pos);
    const retron::player& player = this->level_service_.player(index_in_level);
    const retron::player& player_or_coop = this->level_service_.player_or_coop(index_in_level);
    const ff::input_event_provider& input_events = this->level_service_.game_service().input_events(player);
    this->registry.emplace<::player_data>(entity, player, player_or_coop, input_events, 0u);

    retron::particles::effect_options options;
    options.type = static_cast<uint8_t>(player.index);

    int effect_id = this->particle_effects["player_enter"].add(this->particles, pos, &options);
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

void retron::level::create_objects(size_t count, entity_type type, const ff::rect_fixed& rect, const std::vector<ff::rect_fixed>& avoid_rects, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func)
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
                if (create_func)
                {
                    create_func(type, pos);
                }
                else
                {
                    this->create_entity(type, pos);
                }

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
    ff::point_fixed move_vector = ::get_press_vector(player_data.input_events, false);
    ff::point_fixed shot_vector = ::get_press_vector(player_data.input_events, true);

    this->position.velocity(entity, move_vector * this->difficulty_spec_.player_move);
    this->position.set(entity, this->position.get(entity) + this->position.velocity(entity));
    this->position.direction(entity, ff::point_fixed(
        -ff::fixed_int(move_vector.x < 0_f) + ff::fixed_int(move_vector.x > 0_f),
        -ff::fixed_int(move_vector.y < 0_f) + ff::fixed_int(move_vector.y > 0_f)));

    if ((!player_data.shot_counter || !--player_data.shot_counter) && shot_vector)
    {
        player_data.shot_counter = this->difficulty_spec_.player_shot_counter;
        this->create_player_bullet(entity, this->bounds_box(entity).center(), retron::position::canon_direction(shot_vector));
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
    if (!this->registry.try_get<::particle_effect_follows_entity>(entity))
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
            options.type = static_cast<uint8_t>(player_data->player.get().index);
        }
    }

    this->particle_effects["player_bullet_hit_bounds"].add(this->particles, pos_array.data(), pos_array.size(), &options);
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

                                this->particle_effects["grunt_destroy"].add(
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
    for (auto [entity, data] : this->registry.view<::player_data>().each())
    {
        if (data.player.get().index)
        {
            ff::palette_base& palette = retron::app_service::get().player_palette(data.player.get().index);
            draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

            this->particles.render(draw, static_cast<uint8_t>(data.player.get().index));

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
    if (!this->registry.try_get<::particle_effect_follows_entity>(entity))
    {
        ::player_data& player_data = this->registry.get<::player_data>(entity);
        ff::palette_base& palette = retron::app_service::get().player_palette(player_data.player.get().index);
        draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

        ff::point_fixed dir = this->position.direction(entity);
        ff::fixed_int frame = this->position.velocity(entity) ? ff::fixed_int(this->frame_count) / this->difficulty_spec_.player_move_frame_divisor : 0_f;
        ff::animation_base* anim = this->player_walk_anims[helpers::dir_to_index(dir)].object().get();
        this->render_animation(entity, draw, anim, frame);

        draw.pop_palette_remap();
    }
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
    size_t type = this->registry.get<::electrode_data>(entity).electrode_type;
    this->electrode_anims[type % this->electrode_anims.size()]->draw_frame(draw, ff::pixel_transform(pos), 0);
}

void retron::level::render_hulk(entt::entity entity, ff::draw_base& draw)
{
    ff::point_fixed pos = this->position.get(entity);
    draw.draw_palette_filled_rectangle(this->hit_box(entity), 235);
}

void retron::level::render_grunt(entt::entity entity, ff::draw_base& draw)
{
    if (!this->registry.try_get<::particle_effect_follows_entity>(entity))
    {
        ff::point_fixed pos = this->position.get(entity);
        this->render_animation(entity, draw, this->grunt_walk_anim.object().get(), 0);
    }
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
    retron::render_debug_t render_debug = retron::app_service::get().render_debug();

    if (ff::flags::has(render_debug, retron::render_debug_t::controls))
    {
        const ff::fixed_int radius = 12;
        std::array<int, 2> colors{ 244, 243 };
        std::array<const ff::point_fixed, 2> centers{ ff::point_fixed(24, 24), ff::point_fixed(52, 24) };
        draw.draw_palette_outline_circle(centers[0], radius, 245, 1);
        draw.draw_palette_outline_circle(centers[1], radius, 245, 1);

        size_t i = 0;
        for (auto [entity, data] : this->registry.view<::player_data>().each())
        {
            ff::point_fixed move_vector = ::get_press_vector(data.input_events, false);
            ff::point_fixed shot_vector = ::get_press_vector(data.input_events, true);

            if (move_vector)
            {
                draw.draw_palette_line(centers[0], centers[0] + move_vector * radius, colors[i % colors.size()], 1);
            }

            if (shot_vector)
            {
                draw.draw_palette_line(centers[1], centers[1] + shot_vector * radius, colors[i % colors.size()], 1);
            }

            i++;
        }
    }

    if (ff::flags::has(render_debug, retron::render_debug_t::ai_lines))
    {
        for (auto [entity, data] : this->registry.view<::grunt_data>().each())
        {
            draw.draw_palette_line(this->position.get(entity), data.dest_pos, 245, 1);
        }
    }

    if (ff::flags::has(render_debug, retron::render_debug_t::collision))
    {
        this->collision.render_debug(draw);
    }

    if (ff::flags::has(render_debug, retron::render_debug_t::position))
    {
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
