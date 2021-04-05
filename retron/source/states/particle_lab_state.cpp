#include "pch.h"
#include "source/core/app_service.h"
#include "source/core/render_targets.h"
#include "source/states/particle_lab_state.h"
#include "source/ui/particle_lab_page.xaml.h"

retron::particle_lab_state::particle_lab_state(std::shared_ptr<ff::ui_view> view)
    : view(view)
{
    retron::particle_lab_page* page = Noesis::DynamicCast<retron::particle_lab_page*>(view->content());
    assert(page);

    if (page)
    {
        this->connections.emplace_front(page->destroyed_sink().connect(std::bind(&retron::particle_lab_state::on_page_destroyed, this)));
        this->connections.emplace_front(page->clicked_sink().connect(std::bind(&retron::particle_lab_state::on_mouse_click, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
    }
}

std::shared_ptr<ff::state> retron::particle_lab_state::advance_time()
{
    ff::end_scope_action end_action = this->particles.advance_async();
    return nullptr;
}

void retron::particle_lab_state::render()
{
    retron::render_targets& targets = *retron::app_service::get().render_targets();

    ff::draw_ptr draw = retron::app_service::get().draw_device().begin_draw(
        *targets.target(retron::render_target_types::palette_1),
        targets.depth(retron::render_target_types::palette_1).get(),
        constants::RENDER_RECT,
        constants::RENDER_RECT);

    if (draw)
    {
        this->particles.render(*draw);
    }
}

void retron::particle_lab_state::on_page_destroyed()
{
    this->connections.clear();
}

void retron::particle_lab_state::on_mouse_click(int button, ff::point_float pos, std::string_view name, retron::particles::effect_t& effect)
{
    if (button == VK_LBUTTON)
    {
        effect.add(this->particles, pos.cast<ff::fixed_int>());
    }
}
