#include "pch.h"
#include "source/core/app_service.h"
#include "source/level/components.h"
#include "source/level/entities.h"
#include "source/level/level_render.h"

retron::level_render::level_render(retron::level_render_host& host)
    : host(host)
{
    this->connections.emplace_front(retron::app_service::get().reload_resources_sink().connect(std::bind(&retron::level_render::init_resources, this)));

    this->init_resources();
}

void retron::level_render::render(ff::draw_base& draw)
{
    const entt::registry& registry = this->host.host_registry();
    const retron::difficulty_spec& diff = this->host.host_difficulty_spec();
    size_t frame_count = this->host.host_frame_count();

    for (auto [entity, comp] : registry.view<const retron::comp::rectangle>().each())
    {
        draw.draw_palette_outline_rectangle(comp.rect, comp.color, comp.thickness);
    }

    for (auto [entity, comp, pos] : registry.view<const retron::comp::animation, const retron::comp::position>(entt::exclude_t<retron::comp::flag::render_on_top>()).each())
    {
        const retron::comp::scale* scale = registry.try_get<const retron::comp::scale>(entity);
        const retron::comp::rotation* rot = registry.try_get<const retron::comp::rotation>(entity);

        comp.anim->draw_animation(draw, ff::pixel_transform(pos.position, scale ? scale->scale : ff::point_fixed{ 1, 1 }, rot ? rot->rotation : 0_f));
    }

    for (auto [entity, pos, type] : registry.view<const retron::comp::electrode, const retron::comp::position, const retron::entity_type>().each())
    {
        this->electrode_anims[retron::entity_util::index(type)]->draw_frame(draw, ff::pixel_transform(pos.position), 0);
    }

    for (auto [entity, comp, pos] : registry.view<const retron::comp::grunt, const retron::comp::position>(entt::exclude_t<retron::comp::showing_particle_effect>()).each())
    {
        this->grunt_walk_anim->draw_frame(draw, ff::pixel_transform(pos.position), 0);
    }

    for (auto [entity, comp, pos] : registry.view<const retron::comp::hulk, const retron::comp::position>(entt::exclude_t<retron::comp::showing_particle_effect>()).each())
    {
        this->hulk_walk_anim->draw_frame(draw, ff::pixel_transform(pos.position), 0);
    }

    for (auto [entity, comp, pos, type] : registry.view<const retron::comp::bonus, const retron::comp::position, const retron::entity_type>().each())
    {
        this->bonus_anims[retron::entity_util::index(type)]->draw_frame(draw, ff::pixel_transform(pos.position), 0);
    }

    for (auto [entity, pos, rot] : registry.view<const retron::comp::bullet, const retron::comp::position, const retron::comp::rotation>().each())
    {
        this->player_bullet_anim->draw_frame(draw, ff::pixel_transform(pos.position, { 1, 1 }, rot.rotation), 0);
    }

    for (auto [entity, comp, pos, dir, vel] : registry.view<const retron::comp::player, const retron::comp::position, const retron::comp::direction, const retron::comp::velocity>(entt::exclude_t<retron::comp::showing_particle_effect>()).each())
    {
        ff::animation_base* anim = this->player_walk_anims[retron::helpers::dir_to_index(dir.direction)].object().get();
        ff::fixed_int frame = vel.velocity ? ff::fixed_int(frame_count) / diff.player_move_frame_divisor : 0_f;
        ff::palette_base& palette = retron::app_service::get().player_palette(comp.player.get().index);
        draw.push_palette_remap(palette.index_remap(), palette.index_remap_hash());

        switch (comp.state)
        {
            default:
                if (ff::flags::has(retron::app_service::get().debug_cheats(), retron::debug_cheats_t::invincible) && comp.state_counter % 32 < 16)
                {
                    anim = nullptr;
                }
                break;

            case retron::comp::player::player_state::dead:
                frame = 0;

                if (comp.state_counter >= diff.player_dead_counter)
                {
                    anim = nullptr;
                }
                break;

            case retron::comp::player::player_state::ghost:
                if (comp.state_counter >= diff.player_ghost_warning_counter)
                {
                    if (comp.state_counter % 16 < 8)
                    {
                        anim = nullptr;
                    }
                }
                else if (comp.state_counter % 32 < 16)
                {
                    anim = nullptr;
                }
                break;
        }

        if (anim)
        {
            anim->draw_frame(draw, ff::pixel_transform(pos.position), frame);
        }

        draw.pop_palette_remap();
    }

    for (auto [entity, comp, pos] : registry.view<const retron::comp::animation, const retron::comp::position, const retron::comp::flag::render_on_top>().each())
    {
        const retron::comp::scale* scale = registry.try_get<const retron::comp::scale>(entity);
        const retron::comp::rotation* rot = registry.try_get<const retron::comp::rotation>(entity);

        comp.anim->draw_animation(draw, ff::pixel_transform(pos.position, scale ? scale->scale : ff::point_fixed{ 1, 1 }, rot ? rot->rotation : 0_f));
    }
}

void retron::level_render::init_resources()
{
    this->player_walk_anims[0] = "anim.player_walk_right";
    this->player_walk_anims[1] = "anim.player_walk_right_up";
    this->player_walk_anims[2] = "anim.player_walk_up";
    this->player_walk_anims[3] = "anim.player_walk_left_up";
    this->player_walk_anims[4] = "anim.player_walk_left";
    this->player_walk_anims[5] = "anim.player_walk_left_down";
    this->player_walk_anims[6] = "anim.player_walk_down";
    this->player_walk_anims[7] = "anim.player_walk_right_down";

    this->electrode_anims[0] = "sprites.electrode[0]";
    this->electrode_anims[1] = "sprites.electrode[1]";
    this->electrode_anims[2] = "sprites.electrode[2]";

    this->bonus_anims[static_cast<size_t>(retron::bonus_type::woman)] = "sprites.bonus_woman";
    this->bonus_anims[static_cast<size_t>(retron::bonus_type::man)] = "sprites.bonus_man";
    this->bonus_anims[static_cast<size_t>(retron::bonus_type::girl)] = "sprites.bonus_girl";
    this->bonus_anims[static_cast<size_t>(retron::bonus_type::boy)] = "sprites.bonus_boy";
    this->bonus_anims[static_cast<size_t>(retron::bonus_type::dog)] = "sprites.bonus_dog";

    this->player_bullet_anim = "sprites.player_bullet";

    this->grunt_walk_anim = "sprites.grunt";
    this->hulk_walk_anim = "sprites.hulk";
}
