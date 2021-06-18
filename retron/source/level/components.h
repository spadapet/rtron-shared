#pragma once

namespace retron
{
    struct player;
}

namespace retron::comp::flag
{
    // Entities

    struct pending_delete {};

    // Collision

    struct hit_dirty {};
    struct bounds_dirty {};
    struct grunt_avoid_dirty {};

    // Level

    struct clear_to_win {};
    struct hulk_target {};
    struct render_on_top {};
}

namespace retron::comp
{
    // Position

    struct position
    {
        ff::point_fixed position;
    };

    struct velocity
    {
        ff::point_fixed velocity;
    };

    struct direction
    {
        ff::point_fixed direction;
    };

    struct scale
    {
        ff::point_fixed scale;
    };

    struct rotation
    {
        ff::fixed_int rotation;
    };

    // Collision

    struct box_spec
    {
        ff::rect_fixed rect;
    };

    struct box
    {
        ::b2Body* body;
    };

    struct hit_box_spec : public retron::comp::box_spec {};
    struct bounds_box_spec : public retron::comp::box_spec {};
    struct hit_box : public retron::comp::box {};
    struct bounds_box : public retron::comp::box {};
    struct grunt_avoid_box : public retron::comp::box {};

    // Level

    struct rectangle
    {
        ff::rect_fixed rect;
        ff::fixed_int thickness;
        int color;
    };

    struct player
    {
        enum class player_state
        {
            alive,
            ghost,
            dead,
        };

        std::reference_wrapper<const retron::player> player;
        std::reference_wrapper<const ff::input_event_provider> input_events;
        player_state state;
        size_t state_counter;
        size_t allow_shot_frame;
    };

    struct bullet
    {};

    struct bonus
    {
        size_t turn_frame;
    };

    struct grunt
    {
        size_t index;
        size_t move_frame;
        ff::point_fixed dest_pos; // TODO: Change to target entity?
    };

    struct hulk
    {
        size_t index;
        size_t group;
        entt::entity target_entity; // TODO: Delete, use component instead
        ff::point_fixed force_push;
        bool force_turn;
    };

    struct electrode
    {};

    struct animation
    {
        std::shared_ptr<ff::animation_player_base> anim;
    };

    struct tracked_object
    {
        std::reference_wrapper<size_t> init_object_count;
    };

    struct showing_particle_effect
    {
        int effect_id;
    };

    struct target_entity
    {
        entt::entity entity;
    };
}
