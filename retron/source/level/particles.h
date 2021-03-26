#pragma once

namespace retron
{
    class particles
    {
    public:
        particles();

        ff::end_scope_action advance_async();
        void render(ff::draw_base& draw, uint8_t type = 0);

        bool effect_active(int effect_id) const;
        void effect_position(int effect_id, ff::point_fixed pos);

        struct effect_options
        {
            std::pair<ff::fixed_int, ff::fixed_int> angle = std::make_pair(0, 360);
            ff::point_fixed scale = ff::point_fixed(1, 1);
            ff::fixed_int rotate = 0;
            uint8_t type = 0;
            bool reverse = false;
        };

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

            void add(particles& particles, ff::point_fixed pos, int effect_id, const retron::particles::effect_options& options) const;

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

            unsigned short group;
            unsigned short delay;
            unsigned short life;
            unsigned char internal_type;
            unsigned char type;

            union
            {
                int color_;
                ff::animation_base* anim;
            };
        };

        void advance_block();
        void advance_now();

        unsigned short add_group(const ff::pixel_transform& transform, int effect_id, int count, const std::vector<std::shared_ptr<ff::animation_base>>& animations);
        void release_group(unsigned short group_id);
        const DirectX::XMFLOAT4X4& matrix(unsigned short group_id) const;

        std::vector<particle_t> particles_new;
        std::vector<particle_t> particles_async;
        std::vector<group_t> groups;
        ff::win_handle async_event;

    public:
        class effect_t
        {
        public:
            effect_t() = default;
            effect_t(retron::particles::effect_t&&) = default;
            effect_t(const retron::particles::effect_t&) = default;
            effect_t(const ff::value* value);

            retron::particles::effect_t& operator=(retron::particles::effect_t&&) = default;
            retron::particles::effect_t& operator=(const retron::particles::effect_t&) = default;

            int add(retron::particles& particles, ff::point_fixed pos, const retron::particles::effect_options* options = nullptr) const;
            int add(retron::particles& particles, const ff::point_fixed* pos, size_t posCount, const retron::particles::effect_options* options = nullptr) const;

        private:
            std::vector<spec_t> specs;
        };
    };
}
