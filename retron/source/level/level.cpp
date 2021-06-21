#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/game_service.h"
#include "source/core/render_targets.h"
#include "source/level/components.h"
#include "source/level/entity_type.h"
#include "source/level/entity_util.h"
#include "source/level/level.h"

static const size_t MAX_DELAY_PARTICLES = 128;

retron::level::level(retron::game_service& game_service, const retron::level_spec& level_spec, const std::vector<const retron::player*>& players)
    : game_service(game_service)
    , difficulty_spec_(game_service.difficulty_spec())
    , level_spec_(level_spec)
    , players_(players)
    , entities(this->registry)
    , collision(this->registry)
    , level_logic(*this, this->collision)
    , level_collision_logic(*this, this->entities, this->collision)
    , level_render(*this)
    , phase_(internal_phase_t::init)
    , phase_counter(0)
    , frame_count(0)
{
    this->connections.emplace_front(this->registry.on_construct<retron::entity_type>().connect<&retron::level::handle_entity_created>(this));
    this->connections.emplace_front(this->registry.on_destroy<retron::comp::tracked_object>().connect<&retron::level::handle_tracked_entity_deleted>(this));

    this->ff_connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level::init_resources, this)));
    this->ff_connections.emplace_front(this->particles.effect_done_sink().connect(std::bind(&retron::level::handle_particle_effect_done, this, std::placeholders::_1)));

    this->init_resources();
    this->internal_phase(internal_phase_t::ready);
}

std::shared_ptr<ff::state> retron::level::advance_time()
{
    if (this->phase() == retron::level_phase::playing)
    {
        ff::end_scope_action particle_scope = this->particles.advance_async();
        this->advance_entities();
        this->level_collision_logic.handle_collisions();
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

void retron::level::host_create_particles(std::string_view name, const ff::point_fixed& pos, const retron::particle_effect_options* options)
{
    auto i = this->particle_effects.find(name);
    assert(i != this->particle_effects.end());

    if (i != this->particle_effects.end())
    {
        i->second.add(this->particles, pos, options);
    }
}

void retron::level::host_create_bullet(entt::entity player_entity, ff::point_fixed shot_vector)
{
    retron::entity_type type = retron::entity_util::bullet_for_player(this->entities.type(player_entity));
    ff::point_fixed shot_dir = retron::helpers::canon_dir(shot_vector);
    ff::point_fixed pos = this->bounds_box(player_entity).center() + shot_dir * this->difficulty_spec_.player_shot_start_offset;
    ff::point_fixed vel = shot_dir * this->difficulty_spec_.player_shot_move;

    this->entities.create_bullet(player_entity, pos, vel);
}

void retron::level::host_handle_dead_player(entt::entity entity, const retron::player & player)
{
    if (this->game_service.coop_take_life(player))
    {
        this->entities.delay_delete(entity);
        this->create_player(player);
    }
}

void retron::level::host_add_points(const retron::player& player, size_t points)
{
    this->game_service.player_add_points(player, points);
}

void retron::level::init_resources()
{
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
        this->level_logic.reset();
        this->level_collision_logic.reset();

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
            this->create_objects(object_spec.bonus, retron::entity_util::bonus(object_spec.bonus_type), object_spec.rect, std::bind(&retron::entities::create_bonus, &this->entities, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.electrode, retron::entity_util::electrode(object_spec.electrode_type), object_spec.rect, std::bind(&retron::entities::create_electrode, &this->entities, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.grunt, retron::entity_type::enemy_grunt, object_spec.rect, std::bind(&retron::entities::create_grunt, &this->entities, std::placeholders::_1, std::placeholders::_2));
            this->create_objects(object_spec.hulk, retron::entity_type::enemy_hulk, object_spec.rect, std::bind(&retron::entities::create_hulk, &this->entities, std::placeholders::_1, std::placeholders::_2, hulk_group));

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

entt::entity retron::level::create_player(const retron::player& player)
{
    size_t index_in_level = std::find(this->players_.cbegin(), this->players_.cend(), &player) - this->players_.cbegin();
    ff::point_fixed pos = this->level_spec_.player_start;
    pos.x += index_in_level * 16 - this->players_.size() * 8 + 8;

    entt::entity entity = this->entities.create(retron::entity_util::player(player.index), pos);
    const ff::input_event_provider& input_events = this->game_service.input_events(player);
    retron::comp::player::player_state player_state = (this->phase_ == internal_phase_t::show_players) ? retron::comp::player::player_state::alive : retron::comp::player::player_state::ghost;
    this->registry.emplace<retron::comp::direction>(entity, ff::point_fixed{ 0, 1 });
    this->registry.emplace<retron::comp::velocity>(entity, ff::point_fixed{ 0, 0 });
    this->registry.emplace<retron::comp::player>(entity, player, input_events, player_state, 0u, 0u);
    this->registry.emplace<retron::comp::flag::hulk_target>(entity);

    retron::particle_effect_options options;
    options.type = static_cast<uint8_t>(player.index);

    auto [effect_id, max_life] = this->particle_effects["player_start"].add(this->particles, pos, &options);
    this->registry.emplace<retron::comp::showing_particle_effect>(entity, effect_id);

    return entity;
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

void retron::level::create_start_particles(entt::entity entity)
{
    auto names = retron::entity_util::start_particle_names_0_90(this->entities.type(entity));

    if (names.first.size())
    {
        retron::particle_effect_options options;
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

void retron::level::handle_entity_created(entt::registry& registry, entt::entity entity)
{
    if (this->entities.category(entity) == retron::entity_category::enemy && this->entities.type(entity) != retron::entity_type::enemy_hulk)
    {
        this->registry.emplace<retron::comp::flag::clear_to_win>(entity);
    }
}

void retron::level::handle_tracked_entity_deleted(entt::registry& registry, entt::entity entity)
{
    if (this->phase_ != internal_phase_t::ready)
    {
        retron::comp::tracked_object& data = this->registry.get<retron::comp::tracked_object>(entity);
        size_t& count = data.init_object_count.get();
        assert(count);

        if (count)
        {
            count--;
        }
    }
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
        for (auto [entity, comp, pos] : this->registry.view<const retron::comp::grunt, const retron::comp::position>().each())
        {
            draw.draw_line(pos.position, comp.dest_pos, ff::palette_index_to_color(245), 1);
        }

        for (auto [entity, comp, pos] : this->registry.view<const retron::comp::hulk, const retron::comp::position>().each())
        {
            if (this->registry.valid(comp.target_entity))
            {
                draw.draw_line(pos.position, this->registry.get<const retron::comp::position>(comp.target_entity).position, ff::palette_index_to_color(245), 1);
            }
        }
    }

    if (ff::flags::has(render_debug, retron::render_debug_t::collision))
    {
        this->collision.render_debug(draw);
    }

    if (ff::flags::has(render_debug, retron::render_debug_t::position))
    {
        this->entities.render_debug(draw);
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

void retron::level::internal_phase(internal_phase_t new_phase)
{
    if (this->phase_ != new_phase)
    {
        this->phase_ = new_phase;
        this->phase_counter = 0;
        this->init_entities();
    }
}
