#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/level_service.h"
#include "source/level/level.h"

namespace
{
    enum class player_state
    {
        alive,
        ghost,
        dead,
    };

    struct player_data
    {
        std::reference_wrapper<const retron::player> player;
        std::reference_wrapper<const ff::input_event_provider> input_events;
        ::player_state state;
        size_t state_counter;
        size_t shot_counter;
        size_t index_in_level;
    };

    struct player_bullet_data
    {
        entt::entity player;
    };

    struct tracked_object_data
    {
        std::reference_wrapper<size_t> init_object_count;
    };

    struct clear_to_win_flag
    {};

    struct grunt_data
    {
        size_t index;
        size_t move_counter;
        ff::point_fixed dest_pos; // for debug render
    };

    struct electrode_data
    {
        size_t index;
        size_t electrode_type;
    };

    struct showing_particle_effect
    {
        int effect_id;
        size_t counter;
    };

    struct animation_data
    {
        std::shared_ptr<ff::animation_player_base> anim;
    };

    struct animation_follows_entity
    {
        entt::entity entity;
        ff::point_fixed offset;
    };
}

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

static size_t get_player_index_for_bullet(entt::registry& registry, entt::entity bullet_entity, bool* valid = nullptr)
{
    ::player_bullet_data& bullet_data = registry.get<::player_bullet_data>(bullet_entity);
    ::player_data* player_data = registry.valid(bullet_data.player) ? registry.try_get<::player_data>(bullet_data.player) : nullptr;

    if (valid)
    {
        *valid = (player_data != nullptr);
    }

    return player_data ? player_data->player.get().index : 0;
}

retron::level::level(retron::level_service& level_service, retron::level_spec&& level_spec)
    : level_service_(level_service)
    , game_spec_(retron::app_service::get().game_spec())
    , level_spec_(std::move(level_spec))
    , difficulty_spec_(level_service.game_service().difficulty_spec())
    , phase_(internal_phase_t::init)
    , phase_counter_(0)
    , phase_length(0)
    , frame_count(0)
    , entities(this->registry)
    , position(this->registry)
    , collision(this->registry, this->position, this->entities)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level::init_resources, this)));

    this->init_resources();
    this->internal_phase(internal_phase_t::ready);
}

void retron::level::advance(const ff::rect_fixed& camera_rect)
{
    // Scope for particle async update
    {
        ff::end_scope_action particle_scope = this->particles.advance_async();
        this->enum_entities(std::bind(&retron::level::advance_entity, this, std::placeholders::_1, std::placeholders::_2));
        this->handle_collisions();
    }

    this->advance_entity_followers();
    this->advance_phase();
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

retron::level_phase retron::level::phase() const
{
    switch (this->phase_)
    {
        case internal_phase_t::ready:
            return retron::level_phase::ready;

        case internal_phase_t::won:
            return retron::level_phase::won;

        case internal_phase_t::playing:
            for (auto [entity, data] : this->registry.view<const ::player_data>().each())
            {
                if (data.state != ::player_state::dead || data.state_counter < this->difficulty_spec_.player_dead_counter)
                {
                    return retron::level_phase::playing;
                }
            }

            return retron::level_phase::dead;

        default:
            return retron::level_phase::playing;
    }
}

size_t retron::level::phase_counter() const
{
    switch (this->phase_)
    {
        case internal_phase_t::ready:
        case internal_phase_t::won:
            return this->phase_counter_;

        default:
            return 0;
    }
}

void retron::level::start()
{
    if (this->phase_ == internal_phase_t::ready)
    {
        this->internal_phase(internal_phase_t::show_enemies);
    }
}

const retron::level_spec& retron::level::level_spec() const
{
    return this->level_spec_;
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

    this->electrode_die_anims[0] = "anim.electrode_die[0]";
    this->electrode_die_anims[1] = "anim.electrode_die[1]";
    this->electrode_die_anims[2] = "anim.electrode_die[2]";

    this->player_bullet_anim = "sprites.player_bullet";

    this->grunt_walk_anim = "sprites.grunt";

    ff::dict level_particles_dict = ff::auto_resource_value("level_particles").value()->get<ff::dict>();
    for (std::string_view name : level_particles_dict.child_names())
    {
        this->particle_effects.try_emplace(name, retron::particles::effect_t(level_particles_dict.get(name)));
    }
}

void retron::level::init_entities()
{
    if (this->phase_ == internal_phase_t::ready)
    {
        this->entities.delete_all();

        for (const retron::level_rect& level_rect : this->level_spec_.rects)
        {
            switch (level_rect.type)
            {
                case retron::level_rect::type::bounds:
                    this->create_bounds(level_rect.rect.deflate(constants::LEVEL_BORDER_THICKNESS, constants::LEVEL_BORDER_THICKNESS));
                    break;

                case retron::level_rect::type::box:
                    this->create_box(level_rect.rect);
                    break;
            }
        }
    }

    if (this->phase_ == internal_phase_t::show_enemies)
    {
        for (const retron::level_rect& level_rect : this->level_spec_.rects)
        {
            if (level_rect.type == retron::level_rect::type::safe)
            {
                this->entities.delay_delete(this->create_box(level_rect.rect));
            }
        }

        for (retron::level_objects_spec& object_spec : this->level_spec_.objects)
        {
            this->create_objects(object_spec.bonus_woman, entity_type::bonus_woman, object_spec.rect, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.bonus_man, entity_type::bonus_man, object_spec.rect, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.bonus_child, entity_type::bonus_child, object_spec.rect, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.electrode, entity_type::electrode, object_spec.rect, std::bind(&retron::level::create_electrode, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.hulk, entity_type::hulk, object_spec.rect, std::bind(&retron::level::create_entity, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.grunt, entity_type::grunt, object_spec.rect, std::bind(&retron::level::create_grunt, this, std::placeholders::_1, std::placeholders::_2));
        }
    }

    if (this->phase_ == internal_phase_t::show_players)
    {
        for (size_t i = 0; i < this->level_service_.player_count(); i++)
        {
            this->create_player(i);
        }
    }

    this->entities.flush_delete();
}

ff::rect_fixed retron::level::bounds_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::bounds_box);
}

ff::rect_fixed retron::level::hit_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::hit_box);
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
    this->registry.emplace<::electrode_data>(entity, index, this->level_spec_.electrode_type);

    return entity;
}

entt::entity retron::level::create_grunt(retron::entity_type type, const ff::point_fixed& pos)
{
    const size_t max_delay_particles = 128;

    entt::entity entity = this->create_entity(type, pos);
    size_t grunt_index = this->registry.size<::grunt_data>();
    this->registry.emplace<::grunt_data>(entity, grunt_index, this->pick_grunt_move_counter(), pos);

    retron::particles::effect_options options;
    options.delay = static_cast<int>(grunt_index % max_delay_particles);

    bool vertical = ff::math::random_range(1, 10) > 2 ? true : false;
    ff::point_fixed center = this->bounds_box(entity).center();
    auto [effect_id, max_life] = this->particle_effects[vertical ? "grunt_start_90" : "grunt_start_0"].add(this->particles, center, &options);
    this->registry.emplace<::showing_particle_effect>(entity, effect_id, 0u);

    return entity;
}

entt::entity retron::level::create_player(size_t index_in_level)
{
    ff::point_fixed pos = this->level_spec_.player_start;
    pos.x += index_in_level * 16 - this->level_service_.player_count() * 8 + 8;

    entt::entity entity = this->create_entity(entity_type::player, pos);
    const retron::player& player = this->level_service_.player(index_in_level);
    const ff::input_event_provider& input_events = this->level_service_.game_service().input_events(player);
    ::player_state player_state = (this->phase_ == internal_phase_t::show_players) ? ::player_state::alive : ::player_state::ghost;
    this->registry.emplace<::player_data>(entity, player, input_events, player_state, 0u, 0u, index_in_level);

    retron::particles::effect_options options;
    options.type = static_cast<uint8_t>(player.index);

    auto [effect_id, max_life] = this->particle_effects["player_start"].add(this->particles, pos, &options);
    this->registry.emplace<::showing_particle_effect>(entity, effect_id, 0u);

    return entity;
}

entt::entity retron::level::create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir)
{
    entt::entity entity = this->create_entity(entity_type::player_bullet, shot_pos + shot_dir * this->difficulty_spec_.player_shot_start_offset);
    this->registry.emplace<::player_bullet_data>(entity, player);

    this->position.velocity(entity, shot_dir * this->difficulty_spec_.player_shot_move);
    this->position.rotation(entity, helpers::dir_to_degrees(shot_dir));

    return entity;
}

entt::entity retron::level::create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top)
{
    return this->create_animation(std::make_shared<ff::animation_player>(anim), pos, top);
}

entt::entity retron::level::create_animation(std::shared_ptr<ff::animation_player_base> player, ff::point_fixed pos, bool top)
{
    entt::entity entity = this->entities.create(retron::entity_type::animation_bottom);
    this->position.set(entity, pos);
    this->registry.emplace<::animation_data>(entity, std::move(player));
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

namespace
{
    struct place_random_cache
    {
        retron::collision& collision;
        std::vector<entt::entity> hit_entities;
        std::vector<ff::rect_fixed> hit_rects;
        std::vector<std::pair<ff::fixed_int, ff::fixed_int>> gaps;
    };
}

static bool place_random_y(ff::point_fixed& corner, const ff::point_fixed& size, const ff::rect_fixed& corner_bounds, ::place_random_cache& cache)
{
    cache.hit_rects.clear();
    cache.gaps.clear();

    ff::rect_fixed check_rect(corner.x, corner_bounds.top, corner.x + size.x, corner_bounds.bottom);
    for (entt::entity entity : cache.collision.hit_test(check_rect, cache.hit_entities, retron::entity_box_type::none, retron::collision_box_type::bounds_box))
    {
        ff::rect_fixed box = cache.collision.box(entity, retron::collision_box_type::bounds_box);
        if (box.intersects(check_rect))
        {
            cache.hit_rects.push_back(box);
        }
    }

    std::sort(cache.hit_rects.begin(), cache.hit_rects.end(), [](const ff::rect_fixed& r1, const ff::rect_fixed& r2)
    {
        return r1.top < r2.top;
    });

    ff::fixed_int total = 0;
    {
        ff::fixed_int current_y = check_rect.top;

        for (const ff::rect_fixed& rect : cache.hit_rects)
        {
            if (rect.top > current_y)
            {
                ff::fixed_int height = rect.top - current_y;
                if (height >= size.y)
                {
                    height = (height - size.y) + 1_f;
                    cache.gaps.push_back(std::make_pair(current_y, height));
                    total += height;
                }
            }

            current_y = std::max(current_y, rect.bottom);
        }

        if (current_y + size.y <= check_rect.bottom)
        {
            ff::fixed_int height = (check_rect.bottom - current_y - size.y) + 1_f;
            cache.gaps.push_back(std::make_pair(current_y, height));
            total += height;
        }
    }

    if (total > 0_f)
    {
        ff::fixed_int current_y = 0;
        ff::fixed_int pos = ff::math::random_range(0_f, total);

        for (const auto& gap : cache.gaps)
        {
            if (pos >= current_y && pos < current_y + gap.second)
            {
                corner.y = std::floor(gap.first + pos - current_y);
                return true;
            }

            current_y += gap.second;
        }

        assert(false);
    }

    return false;
}

void retron::level::create_objects(size_t& count, entity_type type, const ff::rect_fixed& bounds, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func)
{
    const size_t max_attempts = 256;
    const ff::rect_fixed& spec = retron::get_hit_box_spec(type);
    const ff::point_fixed size = spec.size();

    if (count > 0 && bounds.width() >= size.x && bounds.height() >= size.y)
    {
        ::place_random_cache cache{ this->collision };
        bool clear_to_win = (retron::box_type(type) == retron::entity_box_type::enemy);

        for (size_t i = 0, original_count = count; i < original_count; i++)
        {
            size_t attempt = 0;
            entt::entity entity = entt::null;

            for (; attempt < max_attempts; attempt++)
            {
                ff::point_fixed corner(std::floor(ff::math::random_range(bounds.left, bounds.right - size.x)), 0);
                if (::place_random_y(corner, size, bounds, cache))
                {
                    entity = create_func(type, corner - spec.top_left());
                    this->registry.emplace<::tracked_object_data>(entity, std::ref(count));

                    if (clear_to_win)
                    {
                        this->registry.emplace<::clear_to_win_flag>(entity);
                    }
                    break;
                }
            }

            if (entity == entt::null)
            {
                count--;
            }
        }
    }
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

        case entity_type::animation_bottom:
        case entity_type::animation_top:
            this->advance_animation(entity);
            break;
    }
}

void retron::level::advance_player(entt::entity entity)
{
    const ::showing_particle_effect* particle_data = this->registry.try_get<::showing_particle_effect>(entity);
    if (this->phase_ == internal_phase_t::playing || (this->phase_ == internal_phase_t::show_players && particle_data && particle_data->counter >= this->difficulty_spec_.player_delay_move_counter))
    {
        ::player_data& player_data = this->registry.get<::player_data>(entity);
        player_data.state_counter++;

        switch (player_data.state)
        {
            case ::player_state::ghost:
                if (player_data.state_counter >= this->difficulty_spec_.player_ghost_counter)
                {
                    player_data.state = ::player_state::alive;
                }
                [[fallthrough]];

            case ::player_state::alive:
                {
                    ff::point_fixed move_vector = ::get_press_vector(player_data.input_events, false);
                    ff::point_fixed shot_vector = ::get_press_vector(player_data.input_events, true);

                    this->position.velocity(entity, move_vector * this->difficulty_spec_.player_move);
                    this->position.set(entity, this->position.get(entity) + this->position.velocity(entity));
                    this->position.direction(entity, ff::point_fixed(
                        -ff::fixed_int(move_vector.x < 0_f) + ff::fixed_int(move_vector.x > 0_f),
                        -ff::fixed_int(move_vector.y < 0_f) + ff::fixed_int(move_vector.y > 0_f)));

                    if ((!player_data.shot_counter || !--player_data.shot_counter) && shot_vector)
                    {
                        player_data.state = ::player_state::alive; // stop being an invincible ghost after shooting
                        player_data.shot_counter = this->difficulty_spec_.player_shot_counter;
                        this->create_player_bullet(entity, this->bounds_box(entity).center(), retron::position::canon_direction(shot_vector));
                    }
                }
                break;

            case ::player_state::dead:
                if (player_data.state_counter >= this->difficulty_spec_.player_dead_counter)
                {
                    // Don't stop the game in coop, keep going until there are no lives left
                    const retron::player& player = player_data.player.get();
                    if (player.coop && this->level_service_.game_service().player_get_life(player.index))
                    {
                        this->entities.delay_delete(entity);
                        this->create_player(player_data.index_in_level);
                    }
                }
                break;
        }
    }
}

void retron::level::advance_player_bullet(entt::entity entity)
{
    if (this->player_active())
    {
        ff::point_fixed pos = this->position.get(entity);
        ff::point_fixed vel = this->position.velocity(entity);
        this->position.set(entity, pos + vel);
    }
}

void retron::level::advance_grunt(entt::entity entity)
{
    if (this->player_active())
    {
        ::grunt_data& data = this->registry.get<::grunt_data>(entity);
        if (!--data.move_counter)
        {
            data.move_counter = this->pick_grunt_move_counter();

            entt::entity dest_entity = this->player_target(data.index);
            if (dest_entity != entt::null)
            {
                data.dest_pos = this->pick_move_destination(entity, dest_entity, retron::collision_box_type::grunt_avoid_box);
            }

            ff::point_fixed grunt_pos = this->position.get(entity);
            ff::point_fixed delta = data.dest_pos - grunt_pos;
            ff::point_fixed vel(
                std::copysign(this->difficulty_spec_.grunt_move, delta.x ? delta.x : (ff::math::random_bool() ? 1 : -1)),
                std::copysign(this->difficulty_spec_.grunt_move, delta.y ? delta.y : (ff::math::random_bool() ? 1 : -1)));

            this->position.set(entity, grunt_pos + vel);
        }
    }
}

void retron::level::advance_animation(entt::entity entity)
{
    ::animation_data& data = this->registry.get<::animation_data>(entity);
    std::forward_list<ff::animation_event> events;
    data.anim->advance_animation(&ff::push_front_collection(events));

    for (const ff::animation_event& event : events)
    {
        if (event.event_id == anim_events::NEW_PARTICLES && event.params)
        {
            std::string name = event.params->get<std::string>("name");
            auto i = this->particle_effects.find(name);
            assert(i != this->particle_effects.end());

            if (i != this->particle_effects.end())
            {
                i->second.add(this->particles, this->position.get(entity));
            }
        }
        else if (event.event_id == anim_events::DELETE_ANIMATION)
        {
            this->entities.delay_delete(entity);
        }
    }
}

void retron::level::advance_entity_followers()
{
    for (auto [entity, data] : this->registry.view<::showing_particle_effect>().each())
    {
        data.counter++;

        if (this->particles.effect_active(data.effect_id))
        {
            this->particles.effect_position(data.effect_id, this->bounds_box(entity).center());
        }
        else
        {
            this->registry.remove<::showing_particle_effect>(entity);
        }
    }

    for (auto [entity, follow, data] : this->registry.view<::animation_follows_entity, ::animation_data>().each())
    {
        if (!this->entities.deleted(follow.entity))
        {
            this->position.set(entity, this->position.get(follow.entity) + follow.offset);
        }
    }
}

void retron::level::advance_phase()
{
    this->frame_count++;

    if (++this->phase_counter_ >= this->phase_length)
    {
        switch (this->phase_)
        {
            case internal_phase_t::show_enemies:
                if (this->registry.empty<::showing_particle_effect>())
                {
                    this->internal_phase(internal_phase_t::show_players);
                }
                break;

            case internal_phase_t::show_players:
                if (this->registry.empty<::showing_particle_effect>())
                {
                    this->internal_phase(internal_phase_t::playing);
                }
                break;

            case internal_phase_t::winning:
                this->internal_phase(internal_phase_t::won);
                break;
        }
    }

    if (this->phase() == retron::level_phase::playing && this->registry.empty<::clear_to_win_flag>())
    {
        this->internal_phase(internal_phase_t::winning);
    }
}

void retron::level::handle_collisions()
{
    if (this->phase_ == internal_phase_t::playing)
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
}

void retron::level::handle_bounds_collision(entt::entity target_entity, entt::entity level_entity)
{
    ff::rect_fixed old_rect = this->bounds_box(target_entity);
    ff::rect_fixed new_rect = old_rect;

    ff::rect_fixed level_rect = this->bounds_box(level_entity);
    entity_type level_type = this->entities.entity_type(level_entity);
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
    ff::point_fixed pos = this->position.get(target_entity) + offset;
    this->position.set(target_entity, pos);

    switch (this->entities.entity_type(target_entity))
    {
        case entity_type::player_bullet:
            this->destroy_player_bullet(target_entity, level_entity, retron::entity_box_type::level);
            break;
    }
}

void retron::level::handle_entity_collision(entt::entity target_entity, entt::entity source_entity)
{
    entity_box_type target_type = this->collision.box_type(target_entity, retron::collision_box_type::hit_box);
    entity_box_type source_type = this->collision.box_type(source_entity, retron::collision_box_type::hit_box);

    switch (target_type)
    {
        case entity_box_type::player:
            switch (source_type)
            {
                case entity_box_type::enemy:
                case entity_box_type::enemy_bullet:
                case entity_box_type::obstacle:
                    {
                        ::player_data& data = this->registry.get<::player_data>(target_entity);
                        if (data.state == ::player_state::alive)
                        {
                            data.state = ::player_state::dead;
                            data.state_counter = 0;
                        }
                    }
                    break;
            }
            break;

        case entity_box_type::bonus:
            switch (source_type)
            {
                case entity_box_type::player:
                    // TODO: Collect (points, anim, die, sound)
                    break;

                case entity_box_type::enemy:
                    switch (this->entities.entity_type(source_entity))
                    {
                        case entity_type::hulk:
                            // TODO: Crushed (anim, die, sound)
                            break;
                    }
                    break;
            }
            break;

        case entity_box_type::enemy:
            switch (source_type)
            {
                case entity_box_type::obstacle:
                    this->destroy_grunt(target_entity, source_entity, source_type);
                    break;

                case entity_box_type::player_bullet:
                    this->destroy_grunt(target_entity, source_entity, source_type);
                    this->player_add_points(source_entity, target_entity);
                    break;
            }
            break;

        case entity_box_type::obstacle:
            switch (source_type)
            {
                case entity_box_type::enemy:
                    this->destroy_obstacle(target_entity, source_entity, source_type);
                    break;

                case entity_box_type::player_bullet:
                    this->destroy_obstacle(target_entity, source_entity, source_type);
                    this->player_add_points(source_entity, target_entity);
                    break;
            }
            break;

        case entity_box_type::player_bullet:
            switch (source_type)
            {
                case entity_box_type::enemy:
                case entity_box_type::enemy_bullet:
                case entity_box_type::obstacle:
                    this->destroy_player_bullet(target_entity, source_entity, source_type);
                    break;
            }
            break;

        case entity_box_type::enemy_bullet:
            switch (source_type)
            {
                case entity_box_type::player_bullet:
                    this->entities.delay_delete(target_entity);
                    this->player_add_points(source_entity, target_entity);
                    break;
            }
            break;
    }
}

void retron::level::destroy_player_bullet(entt::entity bullet_entity, entt::entity by_entity, retron::entity_box_type by_type)
{
    if (this->entities.delay_delete(bullet_entity))
    {
        switch (by_type)
        {
            case retron::entity_box_type::level:
                {
                    ff::point_fixed vel = this->position.velocity(bullet_entity);
                    ff::fixed_int angle = this->position.reverse_velocity_as_angle(bullet_entity);
                    ff::point_fixed pos = this->position.get(bullet_entity);
                    ff::point_fixed pos2(pos.x + (vel.x ? std::copysign(1_f, vel.x) : 0), pos.y + (vel.y ? std::copysign(1_f, vel.y) : 0));

                    retron::particles::effect_options options;
                    options.angle = std::make_pair(angle - 60_f, angle + 60_f);
                    options.type = static_cast<uint8_t>(::get_player_index_for_bullet(this->registry, bullet_entity));
                    this->particle_effects["player_bullet_explode"].add(this->particles, pos, &options);
                    this->particle_effects["player_bullet_smoke"].add(this->particles, pos2);
                }
                break;

            default:
                {
                    ff::point_fixed pos = this->bounds_box(by_entity).center();

                    retron::particles::effect_options options;
                    options.type = static_cast<uint8_t>(::get_player_index_for_bullet(this->registry, bullet_entity));
                    this->particle_effects["player_bullet_explode"].add(this->particles, pos, &options);

                    if (by_type != retron::entity_box_type::obstacle)
                    {
                        this->particle_effects["player_bullet_smoke"].add(this->particles, pos);
                    }
                }
                break;
        }
    }
}

void retron::level::destroy_grunt(entt::entity grunt_entity, entt::entity by_entity, retron::entity_box_type by_type)
{
    if (this->entities.delay_delete(grunt_entity))
    {
        this->remove_tracked_object(grunt_entity);

        retron::particles::effect_options options;
        options.reverse = true;

        std::string_view name;
        ff::point_fixed center = this->bounds_box(grunt_entity).center();
        bool bullet = (by_type == entity_box_type::player_bullet);

        switch (retron::helpers::dir_to_index(this->position.velocity(bullet ? by_entity : grunt_entity)))
        {
            case 0:
            case 4:
                name = "grunt_start_90";
                break;

            case 1:
            case 5:
                options.rotate = 45;
                name = "grunt_start_0";
                break;

            case 2:
            case 6:
                name = "grunt_start_0";
                break;

            case 3:
            case 7:
                options.rotate = -45;
                name = "grunt_start_0";
                break;
        }

        this->particle_effects[name].add(this->particles, center, &options);
    }
}

void retron::level::destroy_obstacle(entt::entity obstacle_entity, entt::entity by_entity, retron::entity_box_type by_type)
{
    if (this->entities.delay_delete(obstacle_entity))
    {
        this->remove_tracked_object(obstacle_entity);

        const ::electrode_data* data = this->registry.try_get<::electrode_data>(obstacle_entity);
        std::shared_ptr<ff::animation_base> anim = this->electrode_die_anims[data ? data->electrode_type % this->electrode_die_anims.size() : 0].object();
        this->create_animation(anim, this->position.get(obstacle_entity), false);
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

        case entity_type::animation_bottom:
        case entity_type::animation_top:
            this->render_animation(entity, draw, this->registry.get<::animation_data>(entity).anim.get());
            break;
    }
}

void retron::level::render_player(entt::entity entity, ff::draw_base& draw)
{
    if (this->phase_ > internal_phase_t::ready && !this->registry.all_of<::showing_particle_effect>(entity))
    {
        ::player_data& player_data = this->registry.get<::player_data>(entity);
        ff::palette_base& palette = retron::app_service::get().player_palette(player_data.player.get().index);
        draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

        ff::point_fixed dir = this->position.direction(entity);
        ff::fixed_int frame = (player_data.state != ::player_state::dead && this->position.velocity(entity))
            ? ff::fixed_int(this->frame_count) / this->difficulty_spec_.player_move_frame_divisor
            : 0_f;

        ff::animation_base* anim = this->player_walk_anims[helpers::dir_to_index(dir)].object().get();
        switch (player_data.state)
        {
            case ::player_state::dead:
                if (player_data.state_counter >= this->difficulty_spec_.player_dead_counter)
                {
                    anim = nullptr;
                }
                break;

            case ::player_state::ghost:
                if (player_data.state_counter >= this->difficulty_spec_.player_ghost_warning_counter)
                {
                    if (player_data.state_counter % 16 < 8)
                    {
                        anim = nullptr;
                    }
                }
                else if (player_data.state_counter % 32 < 16)
                {
                    anim = nullptr;
                }
                break;
        }

        if (anim)
        {
            this->render_animation(entity, draw, anim, frame);
        }

        draw.pop_palette_remap();
    }
}

void retron::level::render_player_bullet(entt::entity entity, ff::draw_base& draw)
{
    ::player_bullet_data& data = this->registry.get<::player_bullet_data>(entity);
    this->render_animation(entity, draw, this->player_bullet_anim.object().get(), 0);
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
    if (!this->registry.all_of<::showing_particle_effect>(entity))
    {
        ff::point_fixed pos = this->position.get(entity);
        this->render_animation(entity, draw, this->grunt_walk_anim.object().get(), 0);
    }
}

void retron::level::render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_base* anim, ff::fixed_int frame)
{
    anim->draw_frame(draw, this->position.pixel_transform(entity), frame);
}

void retron::level::render_animation(entt::entity entity, ff::draw_base& draw, ff::animation_player_base* player)
{
    player->draw_animation(draw, this->position.pixel_transform(entity));
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

bool retron::level::player_active() const
{
    switch (this->phase_)
    {
        case internal_phase_t::show_players:
        case internal_phase_t::playing:
            for (auto [entity, data] : this->registry.view<const ::player_data>().each())
            {
                if (data.state != ::player_state::dead)
                {
                    return true;
                }
            }
            return false;

        default:
            return false;
    }
}

entt::entity retron::level::player_target(size_t enemy_index) const
{
    auto view = this->registry.view<const ::player_data>();
    for (size_t i = enemy_index; i < enemy_index + view.size(); i++)
    {
        entt::entity entity = view.data()[i % view.size()];
        const ::player_data& player_data = this->registry.get<const ::player_data>(entity);

        if (player_data.state == ::player_state::alive)
        {
            return entity;
        }
    }

    return entt::null;
}

void retron::level::player_add_points(entt::entity player_or_bullet, entt::entity destroyed_entity)
{
    size_t player_index = 0;
    size_t points = 0;

    switch (this->entities.entity_type(player_or_bullet))
    {
        case retron::entity_type::player:
            player_index = this->registry.get<::player_data>(player_or_bullet).player.get().index;
            break;

        case retron::entity_type::player_bullet:
            player_index = ::get_player_index_for_bullet(this->registry, player_or_bullet);
            break;

        default:
            return;
    }

    switch (this->entities.entity_type(destroyed_entity))
    {
        case retron::entity_type::grunt:
            points = this->difficulty_spec_.points_grunt;
            break;

        case retron::entity_type::electrode:
            points = this->difficulty_spec_.points_electrode;
            break;

        default:
            return;
    }

    if (points)
    {
        this->level_service_.game_service().player_add_points(player_index, points);
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

void retron::level::remove_tracked_object(entt::entity entity)
{
    ::tracked_object_data* data = this->registry.try_get<::tracked_object_data>(entity);
    if (data)
    {
        size_t& count = data->init_object_count.get();
        assert(count);

        if (count)
        {
            count--;
        }
    }
}

void retron::level::enum_entities(const std::function<void(entt::entity, retron::entity_type)>& func)
{
    this->entities.sorted_entities(this->sorted_entities);

    for (const auto& pair : this->sorted_entities)
    {
        func(pair.first, pair.second);
    }
}

void retron::level::internal_phase(internal_phase_t new_phase)
{
    if (this->phase_ != new_phase)
    {
        this->phase_ = new_phase;
        this->phase_counter_ = 0;

        switch (this->phase_)
        {
            case internal_phase_t::winning:
                this->phase_length = this->difficulty_spec_.player_winning_counter;
                break;

            default:
                this->phase_length = 0;
                break;
        }

        this->init_entities();
    }
}
