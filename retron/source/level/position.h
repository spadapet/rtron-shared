#pragma once

namespace retron
{
    class position
    {
    public:
        position(entt::registry& registry);

        void set(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed get(entt::entity entity);

        void velocity(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed velocity(entt::entity entity);

        void direction(entt::entity entity, const ff::point_fixed& value);
        const ff::point_fixed direction(entt::entity entity);
        static ff::point_fixed canon_direction(const ff::point_fixed& value);

        void scale(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed scale(entt::entity entity);

        void rotation(entt::entity entity, ff::fixed_int value);
        ff::fixed_int rotation(entt::entity entity);

        void render_debug(ff::draw_base& draw);

    private:
        entt::registry& registry;
    };
}
