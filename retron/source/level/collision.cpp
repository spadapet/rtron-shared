#include "pch.h"
#include "source/level/collision.h"
#include "source/level/components.h"
#include "source/level/entities.h"
#include "source/level/position.h"

static constexpr ff::fixed_int PIXEL_TO_WORLD_SCALE = 0.0625;
static constexpr ff::fixed_int WORLD_TO_PIXEL_SCALE = 16;
static constexpr ff::fixed_int LEVEL_BOX_AVOID_SKIN = 0.125;

static const std::array<retron::collision_box_type, static_cast<size_t>(retron::collision_box_type::count)> collision_box_types =
{
    retron::collision_box_type::hit_box,
    retron::collision_box_type::bounds_box,
    retron::collision_box_type::grunt_avoid_box,
};

template<typename T>
static entt::entity entity_from_user_data(const T& data)
{
    return static_cast<entt::entity>(data.pointer);
}

template<typename T>
static T user_data_from_entity(entt::entity entity)
{
    T data;
    data.pointer = static_cast<size_t>(entity);
    return data;
}

static ::b2BodyType get_body_type(retron::entity_type type)
{
    return ff::flags::has_any(type, ff::flags::combine(retron::entity_type::category_electrode, retron::entity_type::category_level))
        ? ::b2_staticBody
        : ::b2_dynamicBody;
}

static ff::rect_fixed box(const ::b2Body* body)
{
    ff::rect_fixed rect{};

    const ::b2Shape* shape = body->GetFixtureList()->GetShape();
    for (int i = 0; i < shape->GetChildCount(); i++)
    {
        ::b2AABB aabb;
        shape->ComputeAABB(&aabb, body->GetTransform(), i);
        ff::rect_fixed childRect = ff::rect_fixed(aabb.lowerBound.x, aabb.lowerBound.y, aabb.upperBound.x, aabb.upperBound.y) * ::WORLD_TO_PIXEL_SCALE;
        rect = !i ? childRect : rect.boundary(childRect);
    }

    return rect;
}

static bool might_overlap(const ::b2Contact* contact)
{
    if (contact->IsEnabled() && contact->IsTouching())
    {
        const ::b2Fixture* f1 = contact->GetFixtureA();
        const ::b2Fixture* f2 = contact->GetFixtureB();

        for (int i1 = 0; i1 < f1->GetShape()->GetChildCount(); i1++)
        {
            for (int i2 = 0; i2 < f2->GetShape()->GetChildCount(); i2++)
            {
                if (::b2TestOverlap(f1->GetShape(), i1, f2->GetShape(), i2, f1->GetBody()->GetTransform(), f2->GetBody()->GetTransform()))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

retron::collision::collision(entt::registry& registry, retron::position& position, retron::entities& entities)
    : position_(position)
    , entities_(entities)
    , registry(registry)
    , hit_filter_(this)
    , bounds_filter_(this)
    , worlds{ ::b2World({ 0, 0 }), ::b2World({ 0, 0 }), ::b2World({ 0, 0 }) }
{
    for (::b2World& world : this->worlds)
    {
        world.SetAllowSleeping(false);

        switch (static_cast<retron::collision_box_type>(&world - this->worlds.data()))
        {
            case retron::collision_box_type::bounds_box:
                world.SetContactFilter(&this->bounds_filter_);
                break;

            case retron::collision_box_type::hit_box:
                world.SetContactFilter(&this->hit_filter_);
                break;
        }
    }

    this->connections.emplace_front(this->registry.on_destroy<retron::comp::hit_box>().connect<&collision::box_removed<retron::comp::hit_box>>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::hit_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::hit_box>>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::hit_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::hit_box>>(this));

    this->connections.emplace_front(this->registry.on_destroy<retron::comp::bounds_box>().connect<&collision::box_removed<retron::comp::bounds_box>>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::bounds_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::bounds_box>>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::bounds_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::bounds_box>>(this));

    // grunt_avoid_box relies on retron::comp::bounds_box_spec
    this->connections.emplace_front(this->registry.on_destroy<retron::comp::grunt_avoid_box>().connect<&collision::box_removed<retron::comp::grunt_avoid_box>>(this));
    this->connections.emplace_front(this->registry.on_destroy<retron::comp::bounds_box>().connect<&collision::bounds_box_removed>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::bounds_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::grunt_avoid_box>>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::bounds_box_spec>().connect<&collision::box_spec_changed<retron::collision_box_type::grunt_avoid_box>>(this));

    this->connections.emplace_front(this->registry.on_construct<retron::comp::position>().connect<&retron::collision::position_changed>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::position>().connect<&retron::collision::position_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::direction>().connect<&retron::collision::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::direction>().connect<&retron::collision::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::scale>().connect<&retron::collision::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::scale>().connect<&retron::collision::scale_changed>(this));
    this->connections.emplace_front(this->registry.on_construct<retron::comp::rotation>().connect<&retron::collision::position_changed>(this));
    this->connections.emplace_front(this->registry.on_update<retron::comp::rotation>().connect<&retron::collision::position_changed>(this));

    this->ff_connections.emplace_front(entities.entity_created_sink().connect(std::bind(&collision::entity_created, this, std::placeholders::_1)));
}

const std::vector<std::pair<entt::entity, entt::entity>>& retron::collision::detect_collisions(
    std::vector<std::pair<entt::entity, entt::entity>>& collisions,
    retron::collision_box_type collision_type)
{
    collisions.clear();
    this->update_dirty_boxes(collision_type);

    ::b2World& world = this->worlds[static_cast<size_t>(collision_type)];
    world.Step(ff::constants::seconds_per_advance_f, 1, 1);

    for (::b2Contact* i = world.GetContactList(); i; i = i->GetNext())
    {
        auto [overlaps, entity_a, entity_b] = this->does_overlap(i, collision_type);
        if (overlaps)
        {
            collisions.emplace_back(entity_a, entity_b);
        }
    }

    return collisions;
}

void retron::collision::hit_test(
    const ff::rect_fixed& bounds,
    ff::push_base<entt::entity>& results,
    retron::entity_category filter,
    retron::collision_box_type collision_type,
    size_t max_hits)
{
    this->update_dirty_boxes(collision_type);

    class callback_t : public ::b2QueryCallback
    {
    public:
        callback_t(retron::entities& entities, ff::push_base<entt::entity>& results, retron::entity_category filter, size_t max_hits)
            : entities(entities)
            , results(results)
            , filter(filter)
            , hit_count(0)
            , max_hits(max_hits)
        {}

        virtual bool ReportFixture(::b2Fixture* fixture) override
        {
            entt::entity entity = ::entity_from_user_data(fixture->GetUserData());
            if (this->filter == retron::entity_category::none || ff::flags::has_any(this->entities.category(entity), this->filter))
            {
                this->hit_count++;
                this->results.push(entity);
            }

            return !this->max_hits || this->hit_count < this->max_hits;
        }

    private:
        const retron::entities& entities;
        ff::push_base<entt::entity>& results;
        retron::entity_category filter;
        size_t hit_count;
        size_t max_hits;
    } callback(this->entities_, results, filter, max_hits);

    ff::rect_fixed world_bounds = bounds * ::PIXEL_TO_WORLD_SCALE;
    ::b2World& world = this->worlds[static_cast<size_t>(collision_type)];
    world.QueryAABB(&callback, { { world_bounds.left, world_bounds.top }, { world_bounds.right, world_bounds.bottom } });
}

std::tuple<entt::entity, ff::point_fixed, ff::point_fixed> retron::collision::ray_test(
    const ff::point_fixed& start,
    const ff::point_fixed& end,
    retron::entity_category filter,
    retron::collision_box_type collision_type)
{
    ff::point_fixed world_start = start * ::PIXEL_TO_WORLD_SCALE;
    ff::point_fixed world_end = end * ::PIXEL_TO_WORLD_SCALE;

    if (world_start == world_end)
    {
        return std::make_tuple(entt::null, ff::point_fixed(0, 0), ff::point_fixed(0, 0));
    }

    this->update_dirty_boxes(collision_type);

    class callback_t : public ::b2RayCastCallback
    {
    public:
        callback_t(retron::entities& entities, const ff::point_fixed& origin_point, retron::entity_category filter, retron::collision_box_type collision_type)
            : entities(entities)
            , filter(filter)
            , collision_type(collision_type)
            , entity(entt::null)
            , origin_point(origin_point)
            , point(ff::point_fixed(0, 0))
            , normal(ff::point_fixed(0, 0))
            , fraction(0)
        {}

        std::tuple<entt::entity, ff::point_fixed, ff::point_fixed> result() const
        {
            return std::make_tuple(this->entity, this->point, this->normal);
        }

    private:
        virtual float ReportFixture(::b2Fixture* fixture, const ::b2Vec2& point, const ::b2Vec2& normal, float fraction)
        {
            // Has to be closer than an existing hit
            if (this->entity != entt::null && fraction >= fraction)
            {
                return -1;
            }

            entt::entity entity = ::entity_from_user_data(fixture->GetUserData());
            retron::entity_category type = this->entities.category(entity);
            if (this->filter != retron::entity_category::none && !ff::flags::has_any(type, this->filter))
            {
                return -1;
            }

            if (ff::flags::has(type, retron::entity_category::level) && ::box(fixture->GetBody()).contains(this->origin_point))
            {
                return -1;
            }

            this->entity = entity;
            this->point = ff::point_fixed(point.x, point.y) * ::WORLD_TO_PIXEL_SCALE;
            this->normal = ff::point_fixed(normal.x, normal.y);
            this->fraction = fraction;

            return fraction;
        }

        const retron::entities& entities;
        retron::entity_category filter;
        retron::collision_box_type collision_type;
        entt::entity entity;
        ff::point_fixed origin_point;
        ff::point_fixed point;
        ff::point_fixed normal;
        float fraction;
    } callback(this->entities_, start, filter, collision_type);

    ::b2World& world = this->worlds[static_cast<size_t>(collision_type)];
    world.RayCast(&callback, { world_start.x, world_start.y }, { world_end.x, world_end.y });

    return callback.result();
}

std::tuple<bool, ff::point_fixed, ff::point_fixed> retron::collision::ray_test(
    entt::entity entity,
    const ff::point_fixed& start,
    const ff::point_fixed& end,
    retron::collision_box_type collision_type)
{
    const ::b2Body* body = this->update_box(entity, collision_type);
    if (body)
    {
        ff::point_fixed world_start = start * ::PIXEL_TO_WORLD_SCALE;
        ff::point_fixed world_end = end * ::PIXEL_TO_WORLD_SCALE;

        if (world_start != world_end)
        {
            ::b2RayCastInput ray_data{ ::b2Vec2{ world_start.x, world_start.y }, ::b2Vec2{ world_end.x, world_end.y }, 1.0f };
            const ::b2Shape* shape = body->GetFixtureList()->GetShape();

            for (int i = 0; i < shape->GetChildCount(); i++)
            {
                ::b2RayCastOutput result{};
                if (shape->RayCast(&result, ray_data, body->GetTransform(), 0))
                {
                    ff::point_fixed hit(
                        ff::fixed_int(ray_data.p1.x + result.fraction * (ray_data.p2.x - ray_data.p1.x)) * ::WORLD_TO_PIXEL_SCALE,
                        ff::fixed_int(ray_data.p1.y + result.fraction * (ray_data.p2.y - ray_data.p1.y)) * ::WORLD_TO_PIXEL_SCALE);

                    return std::make_tuple(true, hit, ff::point_fixed(result.normal.x, result.normal.y));
                }
            }
        }
    }

    return std::make_tuple(false, ff::point_fixed(0, 0), ff::point_fixed(0, 0));
}

void retron::collision::box(entt::entity entity, const ff::rect_fixed& rect, retron::collision_box_type collision_type)
{
    switch (collision_type)
    {
        case retron::collision_box_type::hit_box:
            this->registry.emplace_or_replace<retron::comp::hit_box_spec>(entity, rect);
            break;

        case retron::collision_box_type::bounds_box:
            this->registry.emplace_or_replace<retron::comp::bounds_box_spec>(entity, rect);
            break;

        default:
            assert(false);
            break;
    }
}

void retron::collision::reset_box(entt::entity entity, retron::collision_box_type collision_type)
{
    switch (collision_type)
    {
        case retron::collision_box_type::hit_box:
            this->registry.remove<retron::comp::hit_box_spec>(entity);
            break;

        case retron::collision_box_type::bounds_box:
            this->registry.remove<retron::comp::bounds_box_spec>(entity);
            break;

        default:
            assert(false);
            break;
    }

    this->reset_box_internal(entity, collision_type);
}

ff::rect_fixed retron::collision::box_spec(entt::entity entity, retron::collision_box_type collision_type)
{
    ff::rect_fixed box;

    switch (collision_type)
    {
        case retron::collision_box_type::hit_box:
            {
                retron::comp::box_spec* spec = this->registry.try_get<retron::comp::hit_box_spec>(entity);
                box = spec ? spec->rect : retron::entity_util::hit_box_spec(this->entities_.type(entity));
            }
            break;

        case retron::collision_box_type::bounds_box:
            {
                retron::comp::box_spec* spec = this->registry.try_get<retron::comp::bounds_box_spec>(entity);
                box = spec ? spec->rect : retron::entity_util::bounds_box_spec(this->entities_.type(entity));
            }
            break;

        case retron::collision_box_type::grunt_avoid_box:
            {
                ff::rect_fixed grunt_spec = retron::entity_util::bounds_box_spec(retron::entity_type::enemy_grunt);
                box = this->box_spec(entity, retron::collision_box_type::bounds_box);
                return box.inflate(grunt_spec.right, grunt_spec.bottom, -grunt_spec.left, -grunt_spec.top);
            }
            break;

        default:
            assert(false);
            return {};
    }

    return box * this->position_.scale(entity);
}

ff::rect_fixed retron::collision::box(entt::entity entity, retron::collision_box_type collision_type)
{
    ::b2Body* body = this->update_box(entity, collision_type);
    if (body)
    {
        ff::rect_fixed box = ::box(body);
        return this->needs_level_box_avoid_skin(entity, collision_type) ? box.inflate(::LEVEL_BOX_AVOID_SKIN, ::LEVEL_BOX_AVOID_SKIN) : box;
    }

    ff::point_fixed pos = this->position_.get(entity);
    return ff::rect_fixed(pos, pos);
}

void retron::collision::render_debug(ff::draw_base& draw)
{
    this->render_debug<retron::comp::grunt_avoid_box>(draw, retron::collision_box_type::grunt_avoid_box, 1, 245, 248);
    this->render_debug<retron::comp::bounds_box>(draw, retron::collision_box_type::bounds_box, 2, 245, 248);
    this->render_debug<retron::comp::hit_box>(draw, retron::collision_box_type::hit_box, 1, 252, 232);
}

template<typename BoxType>
void retron::collision::render_debug(ff::draw_base& draw, retron::collision_box_type collision_type, int thickness, int color, int color_hit)
{
    for (auto [entity, hb] : this->registry.view<BoxType>().each())
    {
        bool hit = false;
        for (const ::b2ContactEdge* i = hb.body->GetContactList(); !hit && i; i = i->next)
        {
            hit = std::get<bool>(this->does_overlap(i->contact, collision_type));
        }

        ff::rect_fixed rect = this->box(entity, collision_type);
        draw.draw_palette_outline_rectangle(rect.inflate(thickness / 2, thickness / 2), hit ? color_hit : color, thickness);
    }
}

void retron::collision::reset_box_internal(entt::entity entity, retron::collision_box_type collision_type)
{
    switch (collision_type)
    {
        default:
        case retron::collision_box_type::hit_box:
            this->registry.remove<retron::comp::hit_box>(entity);
            break;

        case retron::collision_box_type::bounds_box:
            this->registry.remove<retron::comp::bounds_box>(entity);
            this->registry.remove<retron::comp::grunt_avoid_box>(entity);
            this->dirty_box(entity, retron::collision_box_type::grunt_avoid_box);
            break;
    }

    this->dirty_box(entity, collision_type);
}

void retron::collision::dirty_box(entt::entity entity, retron::collision_box_type collision_type)
{
    const retron::entity_type type = this->entities_.type(entity);
    if (type != retron::entity_type::none)
    {
        switch (collision_type)
        {
            default:
            case retron::collision_box_type::hit_box:
                this->registry.emplace_or_replace<retron::comp::flag::hit_dirty>(entity);
                break;

            case retron::collision_box_type::bounds_box:
                this->registry.emplace_or_replace<retron::comp::flag::bounds_dirty>(entity);
                break;

            case retron::collision_box_type::grunt_avoid_box:
                if (type == retron::entity_type::level_box)
                {
                    this->registry.emplace_or_replace<retron::comp::flag::grunt_avoid_dirty>(entity);
                }
                break;
        }
    }
}

template<typename BoxType, typename DirtyType>
::b2Body* retron::collision::update_box(entt::entity entity, retron::collision_box_type collision_type)
{
    ff::rect_fixed spec = this->box_spec(entity, collision_type);
    if (!spec)
    {
        this->registry.remove<BoxType>(entity);
        this->registry.remove<DirtyType>(entity);
        return nullptr;
    }

    retron::comp::box& hb = this->registry.get_or_emplace<BoxType>(entity, BoxType{});
    if (!hb.body)
    {
        ff::point_fixed pos = this->position_.get(entity) * ::PIXEL_TO_WORLD_SCALE;

        ::b2BodyDef body_def{};
        body_def.userData = ::user_data_from_entity<::b2BodyUserData>(entity);
        body_def.position.Set(pos.x, pos.y);
        body_def.angle = -ff::math::degrees_to_radians(static_cast<float>(this->position_.rotation(entity)));
        body_def.allowSleep = false;
        body_def.fixedRotation = true;
        body_def.type = ::get_body_type(this->entities_.type(entity));

        ::b2World& world = this->worlds[static_cast<size_t>(collision_type)];
        hb.body = world.CreateBody(&body_def);
    }
    else if (this->registry.all_of<DirtyType>(entity))
    {
        ff::point_fixed pos = this->position_.get(entity) * ::PIXEL_TO_WORLD_SCALE;
        float angle = -ff::math::degrees_to_radians(static_cast<float>(this->position_.rotation(entity)));
        hb.body->SetTransform(::b2Vec2(pos.x, pos.y), angle);
    }

    if (!hb.body->GetFixtureList())
    {
        if (this->needs_level_box_avoid_skin(entity, collision_type))
        {
            spec = spec.deflate(::LEVEL_BOX_AVOID_SKIN, ::LEVEL_BOX_AVOID_SKIN);
        }

        spec = spec * ::PIXEL_TO_WORLD_SCALE;

        ff::point_fixed tl = spec.top_left();
        ff::point_fixed br = spec.bottom_right();

        ::b2PolygonShape polygon_shape;
        ::b2ChainShape chain_shape;
        ::b2FixtureDef fixture_def;
        fixture_def.userData = ::user_data_from_entity<::b2FixtureUserData>(entity);
        fixture_def.isSensor = true;

        if (this->entities_.type(entity) == retron::entity_type::level_bounds)
        {
            // Clockwise
            std::array<::b2Vec2, 4> points =
            {
                ::b2Vec2{ tl.x, tl.y },
                ::b2Vec2{ br.x, tl.y },
                ::b2Vec2{ br.x, br.y },
                ::b2Vec2{ tl.x, br.y },
            };

            chain_shape.CreateLoop(points.data(), 4);
            fixture_def.shape = &chain_shape;
        }
        else
        {
            // Counter-clockwise
            std::array<::b2Vec2, 4> points =
            {
                ::b2Vec2{ tl.x, tl.y },
                ::b2Vec2{ tl.x, br.y },
                ::b2Vec2{ br.x, br.y },
                ::b2Vec2{ br.x, tl.y },
            };

            polygon_shape.Set(points.data(), 4);
            fixture_def.shape = &polygon_shape;
        }

        ::b2Fixture* fixture = hb.body->CreateFixture(&fixture_def);
        fixture->GetShape()->m_radius = 0;
    }

    this->registry.remove<DirtyType>(entity);
    return hb.body;
}

::b2Body* retron::collision::update_box(entt::entity entity, retron::collision_box_type collision_type)
{
    switch (collision_type)
    {
        default:
        case retron::collision_box_type::hit_box:
            return this->update_box<retron::comp::hit_box, retron::comp::flag::hit_dirty>(entity, collision_type);

        case retron::collision_box_type::bounds_box:
            return this->update_box<retron::comp::bounds_box, retron::comp::flag::bounds_dirty>(entity, collision_type);

        case retron::collision_box_type::grunt_avoid_box:
            return (this->entities_.type(entity) == retron::entity_type::level_box)
                ? this->update_box<retron::comp::grunt_avoid_box, retron::comp::flag::grunt_avoid_dirty>(entity, collision_type)
                : this->update_box<retron::comp::bounds_box, retron::comp::flag::bounds_dirty>(entity, collision_type);
    }
}

void retron::collision::update_dirty_boxes(retron::collision_box_type collision_type)
{
    switch (collision_type)
    {
        case retron::collision_box_type::hit_box:
            for (entt::entity entity : this->registry.view<retron::comp::flag::hit_dirty>())
            {
                this->update_box(entity, retron::collision_box_type::hit_box);
            }
            break;

        case retron::collision_box_type::bounds_box:
            for (entt::entity entity : this->registry.view<retron::comp::flag::bounds_dirty>())
            {
                this->update_box(entity, retron::collision_box_type::bounds_box);
            }
            break;

        case retron::collision_box_type::grunt_avoid_box:
            for (entt::entity entity : this->registry.view<retron::comp::flag::grunt_avoid_dirty>())
            {
                this->update_box(entity, retron::collision_box_type::grunt_avoid_box);
            }
            break;

        default:
            assert(false);
            break;
    }
}

bool retron::collision::needs_level_box_avoid_skin(entt::entity entity, retron::collision_box_type collision_type)
{
    return collision_type == retron::collision_box_type::grunt_avoid_box && this->entities_.type(entity) == retron::entity_type::level_box;
}

std::tuple<bool, entt::entity, entt::entity> retron::collision::does_overlap(::b2Contact* contact, retron::collision_box_type collision_type)
{
    if (::might_overlap(contact))
    {
        entt::entity entity_a = ::entity_from_user_data(const_cast<::b2Contact*>(contact)->GetFixtureA()->GetUserData());
        entt::entity entity_b = ::entity_from_user_data(const_cast<::b2Contact*>(contact)->GetFixtureB()->GetUserData());

        if (this->box(entity_a, collision_type).intersects(this->box(entity_b, collision_type)))
        {
            bool a_before_b = this->entities_.type(entity_a) <= this->entities_.type(entity_b);
            return std::make_tuple(true, a_before_b ? entity_a : entity_b, a_before_b ? entity_b : entity_a);
        }
    }

    return std::make_tuple(false, entt::null, entt::null);
}

template<typename T>
void retron::collision::box_removed(entt::registry& registry, entt::entity entity)
{
    retron::comp::box& hb = this->registry.get<T>(entity);
    hb.body->GetWorld()->DestroyBody(hb.body);
    hb.body = nullptr;
}

template<retron::collision_box_type T>
void retron::collision::box_spec_changed(entt::registry& registry, entt::entity entity)
{
    this->reset_box_internal(entity, T);
}

void retron::collision::bounds_box_removed(entt::registry& registry, entt::entity entity)
{
    this->registry.remove<retron::comp::grunt_avoid_box>(entity);
}

void retron::collision::entity_created(entt::entity entity)
{
    this->position_changed(this->registry, entity);
}

void retron::collision::position_changed(entt::registry& registry, entt::entity entity)
{
    for (retron::collision_box_type type : ::collision_box_types)
    {
        this->dirty_box(entity, type);
    }
}

void retron::collision::scale_changed(entt::registry& registry, entt::entity entity)
{
    for (retron::collision_box_type type : ::collision_box_types)
    {
        this->reset_box_internal(entity, type);
    }
}

retron::collision::hit_filter::hit_filter(collision* collision)
    : owner(collision)
{}

bool retron::collision::hit_filter::ShouldCollide(::b2Fixture* fixtureA, ::b2Fixture* fixtureB)
{
    entt::entity entity_a = ::entity_from_user_data(fixtureA->GetUserData());
    entt::entity entity_b = ::entity_from_user_data(fixtureB->GetUserData());

    return
        !owner->entities_.deleted(entity_a) &&
        !owner->entities_.deleted(entity_b) &&
        retron::entity_util::can_hit_box_collide(owner->entities_.type(entity_a), owner->entities_.type(entity_b));
}

retron::collision::bounds_filter::bounds_filter(retron::collision* collision)
    : owner(collision)
{}

bool retron::collision::bounds_filter::ShouldCollide(::b2Fixture* fixtureA, ::b2Fixture* fixtureB)
{
    entt::entity entity_a = ::entity_from_user_data(fixtureA->GetUserData());
    entt::entity entity_b = ::entity_from_user_data(fixtureB->GetUserData());

    return
        !owner->entities_.deleted(entity_a) &&
        !owner->entities_.deleted(entity_b) &&
        retron::entity_util::can_bounds_box_collide(owner->entities_.type(entity_a), owner->entities_.type(entity_b));
}
