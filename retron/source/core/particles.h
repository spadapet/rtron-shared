#pragma once

namespace retron
{
    class particles;

    struct particle_effect_options
    {
        std::pair<ff::fixed_int, ff::fixed_int> angle = std::make_pair(0, 360);
        ff::point_fixed scale = ff::point_fixed(1, 1);
        ff::fixed_int rotate = 0;
        ff::fixed_int spin = 0;
        int delay = 0;
        uint8_t type = 0;
        bool reverse = false;
    };

    class particle_effect_base
    {
    public:
        virtual ~particle_effect_base() = default;

        virtual std::tuple<int, size_t> add(retron::particles& particles, ff::point_fixed pos, const retron::particle_effect_options* options = nullptr) const = 0;
        virtual std::tuple<int, size_t> add(retron::particles& particles, const ff::point_fixed* pos, size_t pos_count, const retron::particle_effect_options* options = nullptr) const = 0;
    };

    class particles
    {
    public:
        particles();

        ff::end_scope_action advance_async();
        void render(ff::draw_base& draw, uint8_t type = 0);

        bool effect_active(int effect_id) const;
        void effect_position(int effect_id, ff::point_fixed pos);
        ff::signal_sink<int>& effect_done_sink();

    private:
        class spec_t
        {
        public:
            spec_t() = default;
            spec_t(retron::particles::spec_t&&) = default;
            spec_t(const retron::particles::spec_t&) = default;
            spec_t(const ff::dict& dict);

            spec_t& operator=(retron::particles::spec_t&&) = default;
            spec_t& operator=(const retron::particles::spec_t&) = default;

            size_t add(particles& particles, ff::point_fixed pos, int effect_id, const retron::particle_effect_options& options) const;

        private:
            std::pair<int, int> count;
            std::pair<int, int> delay;
            std::pair<int, int> life;

            std::pair<ff::fixed_int, ff::fixed_int> dist;
            std::pair<ff::fixed_int, ff::fixed_int> dist_vel;
            std::pair<ff::fixed_int, ff::fixed_int> size;
            std::pair<ff::fixed_int, ff::fixed_int> angle;
            std::pair<ff::fixed_int, ff::fixed_int> angle_vel;
            std::pair<ff::fixed_int, ff::fixed_int> spin;
            std::pair<ff::fixed_int, ff::fixed_int> spin_vel;

            ff::fixed_int rotate;
            ff::point_fixed scale;

            std::vector<int> colors;
            std::vector<std::shared_ptr<ff::animation_base>> animations;

            bool reverse;
            bool has_angle;
        };

        struct group_t
        {
            DirectX::XMFLOAT4X4 matrix;
            ff::pixel_transform transform;
            int refs;
            int effect_id;
            std::vector<std::shared_ptr<ff::animation_base>> animations;
        };

        struct particle_t
        {
            bool is_color() const
            {
                return (this->internal_type & 1) != 0;
            }

            void color(int color)
            {
                this->internal_type |= 1;
                this->color_ = color;
            }

            void animation(std::shared_ptr<ff::animation_base> anim)
            {
                this->anim = anim.get();
            }

            // retron::position of particle within its group
            float angle;
            float angle_vel;
            float dist;
            float dist_vel;

            // Rendering of particle
            float size;
            float spin;
            float spin_vel;
            float timer;

            uint16_t group;
            uint16_t delay;
            uint16_t life;
            uint8_t internal_type;
            uint8_t type;

            union
            {
                int color_;
                ff::animation_base* anim;
            };
        };

        void advance_block();
        void advance_now();

        uint16_t add_group(const ff::pixel_transform& transform, int effect_id, int count, const std::vector<std::shared_ptr<ff::animation_base>>& animations);
        void release_group(uint16_t group_id);
        const DirectX::XMFLOAT4X4& matrix(uint16_t group_id) const;

        std::vector<particle_t> particles_new;
        std::vector<particle_t> particles_async;
        std::vector<group_t> groups;
        ff::win_handle async_event;
        ff::signal<int> effect_done_signal;

    public:
        class effect_t : public retron::particle_effect_base
        {
        public:
            effect_t() = default;
            effect_t(retron::particles::effect_t&&) = default;
            effect_t(const retron::particles::effect_t&) = default;
            effect_t(const ff::value* value);

            retron::particles::effect_t& operator=(retron::particles::effect_t&&) = default;
            retron::particles::effect_t& operator=(const retron::particles::effect_t&) = default;

            virtual std::tuple<int, size_t> add(retron::particles& particles, ff::point_fixed pos, const retron::particle_effect_options* options = nullptr) const override;
            virtual std::tuple<int, size_t> add(retron::particles& particles, const ff::point_fixed* pos, size_t pos_count, const retron::particle_effect_options* options = nullptr) const override;

        private:
            std::vector<spec_t> specs;
        };
    };
}
