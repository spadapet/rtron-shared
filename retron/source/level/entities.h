#pragma once

namespace retron
{
    enum class entity_category;
    enum class entity_type;

    class entities
    {
    public:
        entities(entt::registry& registry);

        retron::entity_type type(entt::entity entity) const;
        retron::entity_category category(entt::entity entity) const;

        entt::entity create(retron::entity_type type);
        entt::entity create(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_animation(std::shared_ptr<ff::animation_base> anim, ff::point_fixed pos, bool top);
        entt::entity create_animation(std::shared_ptr<ff::animation_player_base> anim_player, ff::point_fixed pos, bool top);
        entt::entity create_bonus(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_electrode(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_grunt(retron::entity_type type, const ff::point_fixed& pos);
        entt::entity create_hulk(retron::entity_type type, const ff::point_fixed& pos, size_t group);
        entt::entity create_bullet(entt::entity player, ff::point_fixed pos, ff::point_fixed vel);

        bool delay_delete(entt::entity entity);
        bool deleted(entt::entity entity) const;
        void flush_delete();
        void delete_all();

        // Common component accessors

        void position(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed position(entt::entity entity);

        void velocity(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed velocity(entt::entity entity);

        void direction(entt::entity entity, const ff::point_fixed& value);
        const ff::point_fixed direction(entt::entity entity);

        void scale(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed scale(entt::entity entity);

        void rotation(entt::entity entity, ff::fixed_int value);
        ff::fixed_int rotation(entt::entity entity);

        void render_debug(ff::draw_base& draw);

    private:
        entt::registry& registry;
    };
}
