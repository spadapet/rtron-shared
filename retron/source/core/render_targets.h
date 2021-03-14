#pragma once

namespace retron
{
    enum class render_target_types
    {
        none = 0x00,
        palette_1 = 0x01,
        rgb_pma_2 = 0x02,
    };

    class render_targets
    {
    public:
        render_targets(retron::render_target_types targets);

        void clear();
        void render(ff::dx11_target_base& target);
        const std::shared_ptr<ff::dx11_texture>& texture(render_target_types target) const;
        const std::shared_ptr<ff::dx11_target_base>& target(render_target_types target) const;
        const std::shared_ptr<ff::dx11_depth>& depth(render_target_types target) const;

    private:
        render_target_types targets;
        ff::viewport viewport;
        std::shared_ptr<ff::dx11_depth> depth_;
        std::shared_ptr<ff::dx11_texture> texture_rgb_pma_1;
        std::shared_ptr<ff::dx11_target_base> target_rgb_pma_1;
        std::shared_ptr<ff::dx11_texture> texture_palette_1;
        std::shared_ptr<ff::dx11_target_base> target_palette_1;
        std::shared_ptr<ff::dx11_texture> texture_1080;
        std::shared_ptr<ff::dx11_target_base> target_1080;
    };
}
