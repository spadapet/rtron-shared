#pragma once

namespace retron
{
    enum class bonus_type;
    enum class entity_type;
}

namespace retron::entity_util
{
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
