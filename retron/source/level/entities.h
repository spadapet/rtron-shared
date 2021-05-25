#pragma once

namespace retron
{
    enum class bonus_type;

    // This is added to every entity as a component
    enum class entity_type
    {
        none,
        animation_top,
        player_bullet,
        player,
        bonus_adult,
        bonus_child,
        bonus_pet,
        grunt,
        hulk,
        electrode,
        animation_bottom,
        level_bounds,
        level_box,

        count
    };

    enum class entity_box_type
    {
        none,
        player,
        bonus,
        enemy,
        obstacle,
        player_bullet,
        enemy_bullet,
        level,

        count
    };

    retron::entity_type bonus_entity_type(retron::bonus_type type);
    retron::entity_box_type box_type(retron::entity_type type);

    const ff::rect_fixed& get_hit_box_spec(retron::entity_type type);
    bool can_hit_box_collide(retron::entity_box_type type_a, retron::entity_box_type type_b);

    const ff::rect_fixed& get_bounds_box_spec(retron::entity_type type);
    bool can_bounds_box_collide(retron::entity_box_type type_a, retron::entity_box_type type_b);

    class entities
    {
    public:
        entities(entt::registry& registry);

        entt::entity create(retron::entity_type type);
        bool delay_delete(entt::entity entity);
        bool deleted(entt::entity entity);
        void flush_delete();
        void delete_all();

        const std::vector<std::pair<entt::entity, retron::entity_type>>& sorted_entities(std::vector<std::pair<entt::entity, retron::entity_type>>& pairs);
        retron::entity_type entity_type(entt::entity entity);

        ff::signal_sink<entt::entity>& entity_created_sink();
        ff::signal_sink<entt::entity>& entity_deleting_sink();
        ff::signal_sink<entt::entity>& entity_deleted_sink();

    private:
        entt::registry& registry;
        ff::signal<entt::entity> entity_created_signal;
        ff::signal<entt::entity> entity_deleting_signal;
        ff::signal<entt::entity> entity_deleted_signal;
        bool sort_entities_;
    };
}
