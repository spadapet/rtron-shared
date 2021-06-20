#pragma once

namespace retron
{
    enum class bonus_type;

    enum class entity_category
    {
        none = 0,

        animation = 1 << 16,
        player = 1 << 17,
        bullet = 1 << 18,
        bonus = 1 << 19,
        enemy = 1 << 20,
        electrode = 1 << 21,
        level = 1 << 22,
    };

    enum class entity_type
    {
        none = 0,

        // Categories
        category_mask = 0x7FFF0000,

        category_animation = static_cast<int>(retron::entity_category::animation),
        category_player = static_cast<int>(retron::entity_category::player),
        category_bullet = static_cast<int>(retron::entity_category::bullet),
        category_bonus = static_cast<int>(retron::entity_category::bonus),
        category_enemy = static_cast<int>(retron::entity_category::enemy),
        category_electrode = static_cast<int>(retron::entity_category::electrode),
        category_level = static_cast<int>(retron::entity_category::level),

        // Collisions

        category_player_collision = category_bonus | category_enemy | category_electrode | category_level,
        category_bullet_collision = category_enemy | category_electrode | category_level,
        category_bonus_collision = category_enemy | category_electrode | category_level,
        category_enemy_collision = category_electrode | category_level,

        // Types

        animation_top = category_animation | (1 << 0),
        animation_bottom = category_animation | (1 << 1),
        bullet_player_0 = category_bullet | (1 << 0),
        bullet_player_1 = category_bullet | (1 << 1),
        player_0 = category_player | (1 << 0),
        player_1 = category_player | (1 << 1),
        bonus_woman = category_bonus | (1 << 0),
        bonus_man = category_bonus | (1 << 1),
        bonus_girl = category_bonus | (1 << 2),
        bonus_boy = category_bonus | (1 << 3),
        bonus_dog = category_bonus | (1 << 4),
        enemy_grunt = category_enemy | (1 << 0),
        enemy_hulk = category_enemy | (1 << 1),
        electrode_0 = category_electrode | (1 << 0),
        electrode_1 = category_electrode | (1 << 1),
        electrode_2 = category_electrode | (1 << 2),
        electrode_3 = category_electrode | (1 << 3),
        level_bounds = category_level | (1 << 0),
        level_box = category_level | (1 << 1),
    };

    namespace entity_util
    {
        constexpr retron::entity_category category(retron::entity_type type)
        {
            return static_cast<retron::entity_category>(ff::flags::get(type, retron::entity_type::category_mask));
        }

        ff::rect_fixed hit_box_spec(retron::entity_type type);
        bool can_hit_box_collide(retron::entity_type type_a, retron::entity_type type_b);

        ff::rect_fixed bounds_box_spec(retron::entity_type type);
        bool can_bounds_box_collide(retron::entity_type type_a, retron::entity_type type_b);

        size_t index(retron::entity_type type);
        retron::entity_type bonus(retron::bonus_type type);
        retron::entity_type player(size_t index);
        retron::entity_type electrode(size_t index);
        retron::entity_type bullet_for_player(retron::entity_type type);
        std::pair<std::string_view, std::string_view> start_particle_names_0_90(retron::entity_type type);
    }

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

        ff::signal_sink<entt::entity, retron::entity_type>& entity_created_sink();
        ff::signal_sink<entt::entity>& entity_deleting_sink();
        ff::signal_sink<entt::entity>& entity_deleted_sink();

    private:
        void handle_deleting(entt::registry& registry, entt::entity entity);
        void handle_deleted(entt::registry& registry, entt::entity entity);

        entt::registry& registry;
        std::forward_list<entt::scoped_connection> connections;
        ff::signal<entt::entity, retron::entity_type> entity_created_signal;
        ff::signal<entt::entity> entity_deleting_signal;
        ff::signal<entt::entity> entity_deleted_signal;
    };
}
