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
        ff::fixed_int velocity_as_angle(entt::entity entity);
        ff::fixed_int reverse_velocity_as_angle(entt::entity entity);

        void direction(entt::entity entity, const ff::point_fixed& value);
        const ff::point_fixed direction(entt::entity entity);
        static ff::point_fixed canon_direction(const ff::point_fixed& value);

        void scale(entt::entity entity, const ff::point_fixed& value);
        ff::point_fixed scale(entt::entity entity);

        void rotation(entt::entity entity, ff::fixed_int value);
        ff::fixed_int rotation(entt::entity entity);

        ff::signal_sink<entt::entity>& position_changed_sink();
        ff::signal_sink<entt::entity>& velocity_changed_sink();
        ff::signal_sink<entt::entity>& direction_changed_sink();
        ff::signal_sink<entt::entity>& scale_changed_sink();
        ff::signal_sink<entt::entity>& rotation_changed_sink();

        void render_debug(ff::draw_base& draw);

    private:
        void position_changed(entt::registry& registry, entt::entity entity);
        void velocity_changed(entt::registry& registry, entt::entity entity);
        void direction_changed(entt::registry& registry, entt::entity entity);
        void scale_changed(entt::registry& registry, entt::entity entity);
        void rotation_changed(entt::registry& registry, entt::entity entity);

        entt::registry& registry;
        std::forward_list<entt::scoped_connection> connections;

        ff::signal<entt::entity> position_changed_signal;
        ff::signal<entt::entity> velocity_changed_signal;
        ff::signal<entt::entity> direction_changed_signal;
        ff::signal<entt::entity> scale_changed_signal;
        ff::signal<entt::entity> rotation_changed_signal;
    };
}
