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
        render_targets();

        void clear();
        void render(ff::target_base& target);
        const std::shared_ptr<ff::texture>& texture(render_target_types target);
        const std::shared_ptr<ff::target_base>& target(render_target_types target);
        const std::shared_ptr<ff::depth>& depth(render_target_types target);

    private:
        render_target_types used_targets;
        ff::viewport viewport;
        std::shared_ptr<ff::depth> depth_;
        std::shared_ptr<ff::texture> texture_rgb_pma_1;
        std::shared_ptr<ff::target_base> target_rgb_pma_1;
        std::shared_ptr<ff::texture> texture_palette_1;
        std::shared_ptr<ff::target_base> target_palette_1;
        std::shared_ptr<ff::texture> texture_1080;
        std::shared_ptr<ff::target_base> target_1080;
    };
}
