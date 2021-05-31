#pragma once

namespace retron
{
    class entities;
    class position;
    enum class entity_box_type;

    enum class collision_box_type
    {
        hit_box,
        bounds_box,
        grunt_avoid_box,
        count
    };

    class collision
    {
    public:
        collision(entt::registry& registry, retron::position& position, entities& entities);

        const std::vector<std::pair<entt::entity, entt::entity>>& detect_collisions(std::vector<std::pair<entt::entity, entt::entity>>& collisions, retron::collision_box_type collision_type);
        void hit_test(const ff::rect_fixed& bounds, ff::push_base<entt::entity>& entities, entity_box_type box_type_filter, retron::collision_box_type collision_type, size_t max_hits = 0);
        std::tuple<entt::entity, ff::point_fixed, ff::point_fixed> ray_test(const ff::point_fixed& start, const ff::point_fixed& end, entity_box_type box_type_filter, retron::collision_box_type collision_type);
        std::tuple<bool, ff::point_fixed, ff::point_fixed> ray_test(entt::entity entity, const ff::point_fixed& start, const ff::point_fixed& end, retron::collision_box_type collision_type);

        void box(entt::entity entity, const ff::rect_fixed& rect, entity_box_type type, retron::collision_box_type collision_type);
        void reset_box(entt::entity entity, retron::collision_box_type collision_type);
        ff::rect_fixed box_spec(entt::entity entity, retron::collision_box_type collision_type);
        ff::rect_fixed box(entt::entity entity, retron::collision_box_type collision_type);
        entity_box_type box_type(entt::entity entity, retron::collision_box_type collision_type);

        void render_debug(ff::draw_base& draw);

    private:
        class hit_filter : public ::b2ContactFilter
        {
        public:
            hit_filter(retron::collision* collision);

        private:
            virtual bool ShouldCollide(::b2Fixture* fixtureA, ::b2Fixture* fixtureB) override;

            retron::collision* owner;
        };

        class bounds_filter : public ::b2ContactFilter
        {
        public:
            bounds_filter(retron::collision* collision);

        private:
            virtual bool ShouldCollide(::b2Fixture* fixtureA, ::b2Fixture* fixtureB) override;

            retron::collision* owner;
        };

        void reset_box_internal(entt::entity entity, retron::collision_box_type collision_type);
        void dirty_box(entt::entity entity, retron::collision_box_type collision_type);
        ::b2Body* update_box(entt::entity entity, retron::collision_box_type collision_type);
        void update_dirty_boxes(retron::collision_box_type collision_type);
        bool needs_level_box_avoid_skin(entt::entity entity, retron::collision_box_type collision_type);
        std::tuple<bool, entt::entity, entt::entity> does_overlap(::b2Contact* contact, retron::collision_box_type collision_type);

        void bounds_box_removed(entt::registry& registry, entt::entity entity);
        void entity_created(entt::entity entity);
        void position_changed(entt::entity entity);
        void scale_changed(entt::entity entity);

        template<typename BoxType, typename DirtyType> ::b2Body* update_box(entt::entity entity, retron::entity_box_type type, retron::collision_box_type collision_type);
        template<typename BoxType> void render_debug(ff::draw_base& draw, retron::collision_box_type collision_type, int thickness, int color, int color_hit);
        template<typename T> void box_removed(entt::registry& registry, entt::entity entity);
        template<retron::collision_box_type T> void box_spec_changed(entt::registry& registry, entt::entity entity);

        // Entities
        retron::position& position_;
        retron::entities& entities_;
        entt::registry& registry;
        std::forward_list<entt::scoped_connection> connections;
        std::forward_list<ff::signal_connection> ff_connections;

        // Box2d
        retron::collision::hit_filter hit_filter_;
        retron::collision::bounds_filter bounds_filter_;
        std::array<::b2World, static_cast<size_t>(retron::collision_box_type::count)> worlds;
    };
}
