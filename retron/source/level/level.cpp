#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/render_targets.h"
#include "source/level/components.h"
#include "source/level/level.h"

static const size_t MAX_DELAY_PARTICLES = 128;

static std::pair<std::string_view, std::string_view> start_particle_names_0_90(retron::entity_type type)
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

retron::level::level(retron::game_service& game_service, const retron::level_spec& level_spec, const std::vector<const retron::player*>& players)
    : game_service(game_service)
    , difficulty_spec_(game_service.difficulty_spec())
    , level_spec_(level_spec)
    , players_(players)
    , entities(this->registry)
    , position(this->registry)
    , collision(this->registry, this->position, this->entities)
    , level_logic(*this, this->collision)
    , level_render(*this)
    , phase_(internal_phase_t::init)
    , phase_counter(0)
    , frame_count(0)
    , bonus_collected(0)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level::init_resources, this)));
    this->connections.emplace_front(this->particles.effect_done_sink().connect(std::bind(&retron::level::handle_particle_effect_done, this, std::placeholders::_1)));
    this->connections.emplace_front(this->entities.entity_created_sink().connect(std::bind(&retron::level::handle_entity_created, this, std::placeholders::_1)));
    this->connections.emplace_front(this->entities.entity_deleted_sink().connect(std::bind(&retron::level::handle_entity_deleted, this, std::placeholders::_1)));

    this->init_resources();
    this->internal_phase(internal_phase_t::ready);
}

std::shared_ptr<ff::state> retron::level::advance_time()
{
    if (this->phase() == retron::level_phase::playing)
    {
        ff::end_scope_action particle_scope = this->particles.advance_async();
        this->advance_entities();
        this->handle_collisions();
        this->frame_count++;
    }

    this->entities.flush_delete();
    this->advance_entity_followers();
    this->advance_phase();

    return nullptr;
}

void retron::level::render()
{
    ff::draw_ptr draw = retron::app_service::begin_palette_draw();
    if (draw)
    {
        this->level_render.render(*draw);
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

        case internal_phase_t::game_over:
            return retron::level_phase::game_over;

        case internal_phase_t::playing:
            for (auto [entity, data] : this->registry.view<const retron::comp::player>().each())
            {
                if (data.state != retron::comp::player::player_state::dead || data.state_counter < this->difficulty_spec_.player_dead_counter)
                {
                    return retron::level_phase::playing;
                }
            }

            return retron::level_phase::dead;

        default:
            return retron::level_phase::playing;
    }
}

void retron::level::start()
{
    assert(this->phase() == retron::level_phase::ready);

    if (this->phase() == retron::level_phase::ready)
    {
        this->internal_phase(internal_phase_t::before_show);
    }
}

void retron::level::restart()
{
    assert(this->phase() == retron::level_phase::dead);

    if (this->phase() == retron::level_phase::dead)
    {
        this->internal_phase(internal_phase_t::ready);
    }
}

void retron::level::stop()
{
    assert(this->phase() == retron::level_phase::dead);

    if (this->phase() == retron::level_phase::dead)
    {
        this->internal_phase(internal_phase_t::game_over);
    }
}

const std::vector<const retron::player*>& retron::level::players() const
{
    return this->players_;
}

entt::registry& retron::level::host_registry()
{
    return this->registry;
}

const entt::registry& retron::level::host_registry() const
{
    return this->registry;
}

const retron::difficulty_spec& retron::level::host_difficulty_spec() const
{
    return this->difficulty_spec_;
}

size_t retron::level::host_frame_count() const
{
    return this->frame_count;
}

void retron::level::host_create_particles(std::string_view name, const ff::point_fixed& pos)
{
    auto i = this->particle_effects.find(name);
    assert(i != this->particle_effects.end());

    if (i != this->particle_effects.end())
    {
        i->second.add(this->particles, pos);
    }
}

void retron::level::host_create_bullet(entt::entity player_entity, ff::point_fixed shot_vector)
{
    this->create_player_bullet(player_entity, this->bounds_box(player_entity).center(), retron::position::canon_direction(shot_vector));
}

void retron::level::host_handle_dead_player(entt::entity entity, const retron::player & player)
{
    if (this->game_service.coop_take_life(player))
    {
        this->entities.delay_delete(entity);
        this->create_player(player);
    }
}

void retron::level::init_resources()
{
    this->electrode_die_anims[0] = "anim.electrode_die[0]";
    this->electrode_die_anims[1] = "anim.electrode_die[1]";
    this->electrode_die_anims[2] = "anim.electrode_die[2]";

    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::woman)] = "anim.bonus_die_adult";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::man)] = "anim.bonus_die_adult";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::girl)] = "anim.bonus_die_child";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::boy)] = "anim.bonus_die_child";
    this->bonus_die_anims[static_cast<size_t>(retron::bonus_type::dog)] = "anim.bonus_die_pet";

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
        this->frame_count = 0;
        this->bonus_collected = 0;
        this->level_logic.reset();

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
    else if (this->phase_ == internal_phase_t::show_enemies)
    {
        for (const retron::level_rect& level_rect : this->level_spec_.rects)
        {
            if (level_rect.type == retron::level_rect::type::safe)
            {
                this->entities.delay_delete(this->create_box(level_rect.rect));
            }
        }

        size_t hulk_group = 0;

        for (retron::level_objects_spec& object_spec : this->level_spec_.objects)
        {
            this->create_objects(object_spec.bonus, retron::entity_util::bonus(object_spec.bonus_type), object_spec.rect, std::bind(&retron::level::create_bonus, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.electrode, retron::entity_util::electrode(object_spec.electrode_type), object_spec.rect, std::bind(&retron::level::create_electrode, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.grunt, retron::entity_type::enemy_grunt, object_spec.rect, std::bind(&retron::level::create_grunt, this, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.hulk, retron::entity_type::enemy_hulk, object_spec.rect, std::bind(&retron::level::create_hulk, this, std::placeholders::_1, std::placeholders::_2, hulk_group));

            hulk_group += (object_spec.hulk > 0);
        }
    }
    else if (this->phase_ == internal_phase_t::show_players)
    {
        for (const retron::player* player : this->players_)
        {
            this->create_player(*player);
        }
    }

    this->entities.flush_delete();
}

ff::rect_fixed retron::level::bounds_box(entt::entity entity)
{
    return this->collision.box(entity, retron::collision_box_type::bounds_box);
}

entt::entity retron::level::create_entity(entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->entities.create(type);
    this->position.set(entity, pos);
    return entity;
}

entt::entity retron::level::create_bonus(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create_entity(type, pos);
    this->registry.emplace<retron::comp::bonus>(entity, 0u);
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{});
    this->registry.emplace<retron::comp::flag::hulk_target>(entity);

    return entity;
}

entt::entity retron::level::create_electrode(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create_entity(type, pos);
    this->registry.emplace<retron::comp::electrode>(entity);

    return entity;
}

entt::entity retron::level::create_grunt(retron::entity_type type, const ff::point_fixed& pos)
{
    entt::entity entity = this->create_entity(type, pos);
    this->registry.emplace<retron::comp::grunt>(entity, this->registry.size<retron::comp::grunt>(), 0u, pos);
    return entity;
}

entt::entity retron::level::create_hulk(retron::entity_type type, const ff::point_fixed& pos, size_t group)
{
    entt::entity entity = this->create_entity(type, pos);
    this->registry.emplace<retron::comp::hulk>(entity, this->registry.size<retron::comp::hulk>(), group, entt::null, ff::point_fixed{}, false);
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{});
    return entity;
}

entt::entity retron::level::create_player(const retron::player& player)
{
    size_t index_in_level = std::find(this->players_.cbegin(), this->players_.cend(), &player) - this->players_.cbegin();
    ff::point_fixed pos = this->level_spec_.player_start;
    pos.x += index_in_level * 16 - this->players_.size() * 8 + 8;

    entt::entity entity = this->create_entity(retron::entity_util::player(player.index), pos);
    const ff::input_event_provider& input_events = this->game_service.input_events(player);
    retron::comp::player::player_state player_state = (this->phase_ == internal_phase_t::show_players) ? retron::comp::player::player_state::alive : retron::comp::player::player_state::ghost;
    this->registry.emplace<retron::comp::direction>(entity, ff::point_fixed{ 0, 1 });
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{ 0, 0 });
    this->registry.emplace<retron::comp::player>(entity, player, input_events, player_state, 0u, 0u);
    this->registry.emplace<retron::comp::flag::hulk_target>(entity);

    retron::particles::effect_options options;
    options.type = static_cast<uint8_t>(player.index);

    auto [effect_id, max_life] = this->particle_effects["player_start"].add(this->particles, pos, &options);
    this->registry.emplace<retron::comp::showing_particle_effect>(entity, effect_id);

    return entity;
}

entt::entity retron::level::create_player_bullet(entt::entity player, ff::point_fixed shot_pos, ff::point_fixed shot_dir)
{
    retron::entity_type type = retron::entity_util::bullet_for_player(this->entities.type(player));
    entt::entity entity = this->create_entity(type, shot_pos + shot_dir * this->difficulty_spec_.player_shot_start_offset);
    this->registry.emplace<retron::comp::bullet>(entity);
    this->registry.emplace<retron::comp::rotation>(entity, helpers::dir_to_degrees(shot_dir));
    this->registry.emplace<retron::comp::velocity>(entity, shot_dir * this->difficulty_spec_.player_shot_move);

    return entity;
}

entt::entity retron::level::create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top)
{
    return anim ? this->create_animation(std::make_shared<ff::animation_player>(anim), pos, top) : entt::null;
}

entt::entity retron::level::create_animation(std::shared_ptr<ff::animation_player_base> player, ff::point_fixed pos, bool top)
{
    if (player)
    {
        entt::entity entity = this->create_entity(top ? retron::entity_type::animation_top : retron::entity_type::animation_bottom, pos);
        this->registry.emplace<retron::comp::animation>(entity, std::move(player));

        if (top)
        {
            this->registry.emplace<retron::comp::flag::render_on_top>(entity);
        }

        return entity;
    }

    return entt::null;
}

entt::entity retron::level::create_bounds(const ff::rect_fixed& rect)
{
    entt::entity entity = this->entities.create(retron::entity_type::level_bounds);
    this->collision.box(entity, rect, retron::collision_box_type::bounds_box);
    this->registry.emplace<retron::comp::rectangle>(entity, rect, -constants::LEVEL_BORDER_THICKNESS, colors::LEVEL_BORDER);

    return entity;
}

entt::entity retron::level::create_box(const ff::rect_fixed& rect)
{
    entt::entity entity = this->entities.create(retron::entity_type::level_box);
    this->collision.box(entity, rect, retron::collision_box_type::bounds_box);
    this->registry.emplace<retron::comp::rectangle>(entity, rect, constants::LEVEL_BORDER_THICKNESS, colors::LEVEL_BORDER);

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
    cache.hit_entities.clear();
    cache.hit_rects.clear();
    cache.gaps.clear();

    ff::rect_fixed check_rect(corner.x, corner_bounds.top, corner.x + size.x, corner_bounds.bottom);
    cache.collision.hit_test(check_rect, ff::push_back_collection(cache.hit_entities), retron::entity_category::none, retron::collision_box_type::bounds_box);

    for (entt::entity entity : cache.hit_entities)
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

void retron::level::create_start_particles(entt::entity entity)
{
    auto names = ::start_particle_names_0_90(this->entities.type(entity));

    if (names.first.size())
    {
        retron::particles::effect_options options;
        options.delay = static_cast<int>(this->registry.size<retron::comp::showing_particle_effect>() % ::MAX_DELAY_PARTICLES);

        bool vertical = ff::math::random_range(1, 10) > 2 ? true : false;
        ff::point_fixed center = this->bounds_box(entity).center();
        auto [effect_id, max_life] = this->particle_effects[(vertical && names.second.size()) ? names.second : names.first].add(this->particles, center, &options);
        this->registry.emplace<retron::comp::showing_particle_effect>(entity, effect_id);
    }
}

void retron::level::create_objects(size_t& count, retron::entity_type type, const ff::rect_fixed& bounds, const std::function<entt::entity(retron::entity_type, const ff::point_fixed&)>& create_func)
{
    const size_t max_attempts = 256;
    const ff::rect_fixed& spec = retron::entity_util::hit_box_spec(type);
    const ff::point_fixed size = spec.size();

    if (count > 0 && type != retron::entity_type::none && bounds.width() >= size.x && bounds.height() >= size.y)
    {
        ::place_random_cache cache{ this->collision };

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
                    this->registry.emplace<retron::comp::tracked_object>(entity, std::ref(count));
                    this->create_start_particles(entity);
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

void retron::level::advance_entities()
{
    bool player_active = this->player_active();
    retron::entity_category categories = retron::entity_category::animation;
    categories = ff::flags::set(categories, retron::entity_category::enemy, this->phase_ == internal_phase_t::playing && player_active);
    categories = ff::flags::set(categories, retron::entity_category::bonus, this->phase_ < internal_phase_t::show_players || player_active);
    categories = ff::flags::set(categories, retron::entity_category::bullet, player_active);
    categories = ff::flags::set(categories, retron::entity_category::player, this->phase_ == internal_phase_t::playing || this->phase_ == internal_phase_t::show_players);

    this->level_logic.advance_time(categories);
}

void retron::level::advance_entity_followers()
{
    for (auto [entity, comp] : this->registry.view<retron::comp::showing_particle_effect>().each())
    {
        this->particles.effect_position(comp.effect_id, this->bounds_box(entity).center());
    }
}

void retron::level::advance_phase()
{
    this->phase_counter++;

    switch (this->phase_)
    {
        case internal_phase_t::before_show:
            this->internal_phase(internal_phase_t::show_enemies);
            break;

        case internal_phase_t::show_enemies:
            if (this->registry.empty<retron::comp::showing_particle_effect>())
            {
                this->internal_phase(internal_phase_t::show_players);
            }
            break;

        case internal_phase_t::show_players:
            if (this->registry.empty<retron::comp::showing_particle_effect>())
            {
                this->internal_phase(internal_phase_t::playing);
            }
            break;

        case internal_phase_t::winning:
            if (this->phase_counter >= this->difficulty_spec_.player_winning_counter)
            {
                this->internal_phase(internal_phase_t::won);
            }
            break;
    }

    if (this->phase() == retron::level_phase::playing)
    {
        bool winning = this->registry.empty<retron::comp::flag::clear_to_win>();

        if (ff::flags::has(retron::app_service::get().debug_cheats(), retron::debug_cheats_t::complete_level))
        {
            retron::app_service::get().debug_cheats(ff::flags::clear(retron::app_service::get().debug_cheats(), retron::debug_cheats_t::complete_level));
            winning = true;
        }

        if (winning)
        {
            this->internal_phase(internal_phase_t::winning);
        }
    }
}

void retron::level::handle_particle_effect_done(int effect_id)
{
    for (auto [entity, comp] : this->registry.view<retron::comp::showing_particle_effect>().each())
    {
        if (comp.effect_id == effect_id)
        {
            this->registry.remove<retron::comp::showing_particle_effect>(entity);
        }
    }
}

void retron::level::handle_entity_created(entt::entity entity)
{
    retron::entity_type type = this->entities.type(entity);

    if (retron::entity_util::category(type) == retron::entity_category::enemy && type != retron::entity_type::enemy_hulk)
    {
        this->registry.emplace<retron::comp::flag::clear_to_win>(entity);
    }
}

void retron::level::handle_entity_deleted(entt::entity entity)
{
    if (this->phase_ != internal_phase_t::ready)
    {
        retron::comp::tracked_object* data = this->registry.try_get<retron::comp::tracked_object>(entity);
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
}

void retron::level::handle_collisions()
{
    if (this->phase() == retron::level_phase::playing)
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
    ff::rect_fixed new_rect = (this->entities.type(level_entity) == retron::entity_type::level_bounds)
        ? old_rect.move_inside(this->bounds_box(level_entity))
        : old_rect.move_outside(this->bounds_box(level_entity));

    ff::point_fixed offset = new_rect.top_left() - old_rect.top_left();
    this->position.set(target_entity, this->position.get(target_entity) + offset);

    switch (this->entities.category(target_entity))
    {
        case retron::entity_category::bullet:
            this->destroy_player_bullet(target_entity, level_entity);
            break;

        case retron::entity_category::enemy:
            if (offset && this->entities.type(target_entity) == retron::entity_type::enemy_hulk)
            {
                this->registry.get<retron::comp::hulk>(target_entity).force_turn = true;
            }
            break;

        case retron::entity_category::bonus:
            if (offset)
            {
                this->registry.get<retron::comp::bonus>(target_entity).turn_frame = 0;
            }
            break;
    }
}

void retron::level::handle_entity_collision(entt::entity target_entity, entt::entity source_entity)
{
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
                        retron::comp::player& data = this->registry.get<retron::comp::player>(target_entity);
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
                    this->player_add_points(source_entity, target_entity);
                    break;

                case retron::entity_category::enemy:
                    if (source_type == retron::entity_type::enemy_hulk && !this->registry.all_of<retron::comp::showing_particle_effect>(source_entity))
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
                        this->player_add_points(source_entity, target_entity);
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
                    this->player_add_points(source_entity, target_entity);
                    break;
            }
            break;

        case retron::entity_category::bullet:
            switch (source_category)
            {
                case retron::entity_category::electrode:
                    this->destroy_player_bullet(target_entity, source_entity);
                    break;

                case retron::entity_category::enemy:
                    if (source_type == retron::entity_type::enemy_hulk)
                    {
                        this->handle_bounds_collision(target_entity, source_entity);
                    }
                    else
                    {
                        this->destroy_player_bullet(target_entity, source_entity);
                    }
                    break;
            }
            break;
    }
}

void retron::level::destroy_player_bullet(entt::entity bullet_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(bullet_entity))
    {
        retron::entity_category by_category = this->entities.category(by_entity);
        if (by_category != retron::entity_category::enemy)
        {
            ff::point_fixed vel = this->position.velocity(bullet_entity);
            ff::fixed_int angle = ff::math::radians_to_degrees(std::atan2(-vel));
            ff::point_fixed pos = this->position.get(bullet_entity);
            ff::point_fixed pos2(pos.x + (vel.x ? std::copysign(1_f, vel.x) : 0), pos.y + (vel.y ? std::copysign(1_f, vel.y) : 0));

            retron::particles::effect_options options;
            options.angle = std::make_pair(angle - 60_f, angle + 60_f);
            options.type = static_cast<uint8_t>(retron::entity_util::index(this->entities.type(bullet_entity)));
            this->particle_effects["player_bullet_explode"].add(this->particles, pos, &options);
            this->particle_effects["player_bullet_smoke"].add(this->particles, pos2);
        }
        else
        {
            ff::point_fixed pos = this->bounds_box(by_entity).center();

            retron::particles::effect_options options;
            options.type = static_cast<uint8_t>(retron::entity_util::index(this->entities.type(bullet_entity)));
            this->particle_effects["player_bullet_explode"].add(this->particles, pos, &options);

            if (by_category != retron::entity_category::electrode)
            {
                this->particle_effects["player_bullet_smoke"].add(this->particles, pos);
            }
        }
    }
}

void retron::level::destroy_enemy(entt::entity entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(entity))
    {
        auto names = ::start_particle_names_0_90(this->entities.type(entity));
        if (names.first.size())
        {
            retron::particles::effect_options options;
            options.reverse = true;

            std::string_view name;
            ff::point_fixed center = this->bounds_box(entity).center();
            bool by_bullet = this->entities.category(by_entity) == retron::entity_category::bullet;

            switch (retron::helpers::dir_to_index(this->position.velocity(by_bullet ? by_entity : entity)))
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
                this->particle_effects[name].add(this->particles, center, &options);
            }
        }
    }
}

void retron::level::destroy_electrode(entt::entity electrode_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(electrode_entity))
    {
        retron::entity_type electrode_type = this->entities.type(electrode_entity);
        std::shared_ptr<ff::animation_base> anim = this->electrode_die_anims[retron::entity_util::index(electrode_type)].object();

        this->create_animation(anim, this->position.get(electrode_entity), false);
    }
}

void retron::level::destroy_bonus(entt::entity bonus_entity, entt::entity by_entity)
{
    if (this->entities.delay_delete(bonus_entity))
    {
        retron::entity_type type = this->entities.type(bonus_entity);
        std::shared_ptr<ff::animation_base> anim = this->bonus_die_anims[retron::entity_util::index(type)].object();
        this->create_animation(anim, this->position.get(bonus_entity), true);
    }
}

void retron::level::push_hulk(entt::entity enemy_entity, entt::entity by_entity)
{
    ff::point_fixed by_vel = this->position.velocity(by_entity);
    this->registry.get<retron::comp::hulk>(enemy_entity).force_push = ff::point_fixed(
        this->difficulty_spec_.hulk_push.x * (by_vel.x == 0_f ? 0_f : (by_vel.x < 0_f ? -1_f : 1_f)),
        this->difficulty_spec_.hulk_push.y * (by_vel.y == 0_f ? 0_f : (by_vel.y < 0_f ? -1_f : 1_f)));
}

void retron::level::render_particles(ff::draw_base& draw)
{
    // Render particles for the default palette, which includes player 0
    this->particles.render(draw);

    // Render particles for other players
    for (auto [entity, data] : this->registry.view<retron::comp::player>().each())
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
        for (auto [entity, data] : this->registry.view<retron::comp::player>().each())
        {
            ff::point_fixed move_vector = retron::helpers::get_press_vector(data.input_events, false);
            ff::point_fixed shot_vector = retron::helpers::get_press_vector(data.input_events, true);

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
        for (auto [entity, data] : this->registry.view<retron::comp::grunt>().each())
        {
            draw.draw_line(this->position.get(entity), data.dest_pos, ff::palette_index_to_color(245), 1);
        }

        for (auto [entity, data] : this->registry.view<retron::comp::hulk>().each())
        {
            if (this->registry.valid(data.target_entity))
            {
                draw.draw_line(this->position.get(entity), this->position.get(data.target_entity), ff::palette_index_to_color(245), 1);
            }
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
            for (auto [entity, data] : this->registry.view<const retron::comp::player>().each())
            {
                if (data.state != retron::comp::player::player_state::dead)
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
    auto view = this->registry.view<const retron::comp::player>();
    for (size_t i = enemy_index; i < enemy_index + view.size(); i++)
    {
        entt::entity entity = view.data()[i % view.size()];
        const retron::comp::player& player_data = this->registry.get<const retron::comp::player>(entity);

        if (player_data.state == retron::comp::player::player_state::alive)
        {
            return entity;
        }
    }

    return entt::null;
}

void retron::level::player_add_points(entt::entity player_or_bullet, entt::entity destroyed_entity)
{
    size_t points = 0;
    const retron::player* player = nullptr;

    for (auto [e, p] : this->registry.view<retron::comp::player>().each())
    {
        if (p.player.get().index == retron::entity_util::index(this->entities.type(player_or_bullet)))
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
                    points = this->difficulty_spec_.points_grunt;
                    break;
            }
            break;

        case retron::entity_category::electrode:
            points = this->difficulty_spec_.points_electrode;
            break;

        case retron::entity_category::bonus:
            points = this->difficulty_spec_.bonus_points[this->bonus_collected];
            this->bonus_collected = std::min(this->bonus_collected + 1, this->difficulty_spec_.bonus_points.size() - 1);
            break;

        default:
            return;
    }

    if (player && points)
    {
        this->game_service.player_add_points(*player, points);
    }
}

size_t retron::level::pick_grunt_move_frame()
{
    size_t i = std::min<size_t>(this->frame_count / this->difficulty_spec_.grunt_max_ticks_rate, this->difficulty_spec_.grunt_max_ticks - 1);
    i = ff::math::random_range(1u, this->difficulty_spec_.grunt_max_ticks - i) * this->difficulty_spec_.grunt_tick_frames;
    return this->frame_count + std::max<size_t>(i, this->difficulty_spec_.grunt_min_ticks);
}

ff::point_fixed retron::level::pick_move_destination(entt::entity entity, entt::entity dest_entity, retron::collision_box_type collision_type)
{
    ff::point_fixed entity_pos = this->position.get(entity);
    ff::point_fixed dest_pos = this->position.get(dest_entity);
    ff::point_fixed result = dest_pos;

    // Fix the case where the player's foot can get inside of a grunt-avoid box around a level box
    // (since the player's bounding box could be smaller than a grunt)
    {
        ff::stack_vector<entt::entity, 8> box_hits;
        this->collision.hit_test(ff::rect_fixed(dest_pos, dest_pos), ff::push_back_collection(box_hits), retron::entity_category::level, collision_type);

        for (entt::entity box_hit : box_hits)
        {
            if (this->entities.type(box_hit) == retron::entity_type::level_box)
            {
                dest_pos = this->collision.box(box_hit, collision_type).move_point_outside(dest_pos);
            }
        }
    }

    auto [box_entity, box_hit_pos, box_hit_normal] = this->collision.ray_test(entity_pos, dest_pos, retron::entity_category::level, collision_type);
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
                auto [e2, p2, n2] = this->collision.ray_test(entity_pos, corner, retron::entity_category::level, collision_type);
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

entt::entity retron::level::pick_hulk_target(entt::entity entity)
{
    entt::entity target_entity = entt::null;
    ff::fixed_int target_dist = -1;
    ff::point_fixed pos = this->position.get(entity);

    for (entt::entity cur_entity : this->registry.view<retron::comp::flag::hulk_target>())
    {
        ff::point_fixed cur_pos = this->position.get(cur_entity);
        ff::fixed_int cur_dist = (cur_pos - pos).length_squared();

        if (target_dist < 0_f || cur_dist < target_dist)
        {
            target_entity = cur_entity;
            target_dist = cur_dist;
        }
    }

    return target_entity;
}

void retron::level::internal_phase(internal_phase_t new_phase)
{
    if (this->phase_ != new_phase)
    {
        this->phase_ = new_phase;
        this->phase_counter = 0;
        this->init_entities();
    }
}
