#include "pch.h"
#include "source/core/game_spec.h"

template<typename ValueType>
static auto get_value(const ff::dict& dict, const ff::dict& default_dict, std::string_view name)
{
    return dict.get<ValueType>(name, default_dict.get<ValueType>(name));
}

static retron::difficulty_spec load_difficulty_spec(const ff::dict& dict, const ff::dict& default_dict)
{
    retron::difficulty_spec diff_spec{};
    diff_spec.name = ::get_value<std::string>(dict, default_dict, "name");
    diff_spec.level_set = ::get_value<std::string>(dict, default_dict, "level_set");
    diff_spec.lives = ::get_value<size_t>(dict, default_dict, "lives");
    diff_spec.grunt_max_ticks = ::get_value<size_t>(dict, default_dict, "grunt_max_ticks");
    diff_spec.grunt_min_ticks = ::get_value<size_t>(dict, default_dict, "grunt_min_ticks");
    diff_spec.grunt_max_ticks_rate = ::get_value<size_t>(dict, default_dict, "grunt_max_ticks_rate");
    diff_spec.grunt_tick_frames = ::get_value<size_t>(dict, default_dict, "grunt_tick_frames");
    diff_spec.grunt_move = ::get_value<ff::fixed_int>(dict, default_dict, "grunt_move");
    diff_spec.player_move = ::get_value<ff::fixed_int>(dict, default_dict, "player_move");
    diff_spec.player_move_frame_divisor = ::get_value<ff::fixed_int>(dict, default_dict, "player_move_frame_divisor");
    diff_spec.player_shot_move = ::get_value<ff::fixed_int>(dict, default_dict, "player_shot_move");
    diff_spec.player_shot_counter = ::get_value<size_t>(dict, default_dict, "player_shot_counter");

    return diff_spec;
}

static retron::level_set_spec load_level_set_spec(const ff::dict& dict)
{
    retron::level_set_spec level_set_spec{};
    level_set_spec.levels = dict.get<std::vector<std::string>>("levels");
    level_set_spec.loop_start = dict.get<size_t>("loop_start");

    return level_set_spec;
}

static retron::level_objects_spec load_level_objects_spec(ff::rect_fixed rect, const std::vector<ff::value_ptr>& properties)
{
    retron::level_objects_spec objects_spec{};
    objects_spec.type = retron::level_rect::type::objects;
    objects_spec.rect = rect;

    for (ff::value_ptr property_value : properties)
    {
        const ff::dict& property_dict = property_value->get<ff::dict>();
        std::string property_name = property_dict.get<std::string>("name");
        size_t property_value = property_dict.get<size_t>("value");

        if (property_name == "electrode")
        {
            objects_spec.electrode = property_value;
        }
        else if (property_name == "grunt")
        {
            objects_spec.grunt = property_value;
        }
        else if (property_name == "hulk")
        {
            objects_spec.hulk = property_value;
        }
        else if (property_name == "bonus_woman")
        {
            objects_spec.bonus_woman = property_value;
        }
        else if (property_name == "bonus_man")
        {
            objects_spec.bonus_man = property_value;
        }
        else if (property_name == "bonus_child")
        {
            objects_spec.bonus_child = property_value;
        }
        else
        {
            assert(false);
        }
    }

    return objects_spec;
}

static void load_level_spec_object_layer(retron::level_spec& level_spec, const ff::dict& layer_dict)
{
    for (ff::value_ptr object_value : layer_dict.get<std::vector<ff::value_ptr>>("objects"))
    {
        const ff::dict& object_dict = object_value->get<ff::dict>();
        std::string object_type = object_dict.get<std::string>("type");
        if (object_type == "player")
        {
            if (object_dict.get<bool>("point"))
            {
                level_spec.player_start = ff::point_fixed(object_dict.get<ff::fixed_int>("x"), object_dict.get<ff::fixed_int>("y"));
            }
        }
        else
        {
            ff::point_fixed pos(object_dict.get<ff::fixed_int>("x"), object_dict.get<ff::fixed_int>("y"));
            ff::point_fixed size(object_dict.get<ff::fixed_int>("width"), object_dict.get<ff::fixed_int>("height"));
            retron::level_rect level_rect{};
            level_rect.rect = ff::rect_fixed(pos, pos + size);

            if (object_type == "bounds")
            {
                level_rect.type = retron::level_rect::type::bounds;
                level_spec.rects.push_back(level_rect);
            }
            else if (object_type == "box")
            {
                level_rect.type = retron::level_rect::type::box;
                level_spec.rects.push_back(level_rect);
            }
            else if (object_type == "safe")
            {
                level_rect.type = retron::level_rect::type::safe;
                level_spec.rects.push_back(level_rect);
            }
            else if (object_type == "objects")
            {
                level_rect.type = retron::level_rect::type::objects;
                retron::level_objects_spec objects_spec = ::load_level_objects_spec(level_rect.rect, object_dict.get<std::vector<ff::value_ptr>>("properties"));
                level_spec.objects.push_back(std::move(objects_spec));
            }
            else
            {
                assert(false);
            }

            assert(level_rect.rect.area());
        }
    }
}

static void load_level_spec_layer(retron::level_spec& level_spec, const ff::dict& layer_dict)
{
    if (layer_dict.get<std::string>("type") == "objectgroup")
    {
        ::load_level_spec_object_layer(level_spec, layer_dict);
    }
}

static retron::level_spec load_level_spec(const ff::dict& dict)
{
    retron::level_spec level_spec{};
    level_spec.player_start = ff::point_fixed(0, 0);

    for (ff::value_ptr layer_value : dict.get<std::vector<ff::value_ptr>>("layers"))
    {
        ::load_level_spec_layer(level_spec, layer_value->get<ff::dict>());
    }

    return level_spec;
}

retron::game_spec retron::game_spec::load()
{
    ff::auto_resource<ff::resource_value_provider> values_res = ff::global_resources::get("game_spec");
    std::shared_ptr<ff::resource_value_provider> values = values_res.object();

    ff::dict app_dict = values->get_resource_value("app")->get<ff::dict>();
    ff::dict diffs_dict = values->get_resource_value("difficulties")->get<ff::dict>();
    ff::dict level_sets_dict = values->get_resource_value("level_sets")->get<ff::dict>();
    ff::dict levels_dict = values->get_resource_value("levels")->get<ff::dict>();
    ff::dict default_diff_dict = diffs_dict.get<ff::dict>("default");

    retron::game_spec spec{};
    spec.allow_debug = app_dict.get<bool>("allow_debug");
    spec.joystick_min = app_dict.get<ff::fixed_int>("joystick_min");
    spec.joystick_max = app_dict.get<ff::fixed_int>("joystick_max");

    for (std::string_view name : diffs_dict.child_names())
    {
        if (name != "default"sv)
        {
            retron::difficulty_spec diff_spec = ::load_difficulty_spec(diffs_dict.get<ff::dict>(name), default_diff_dict);
            spec.difficulties.try_emplace(std::string(name), std::move(diff_spec));
        }
    }

    for (std::string_view name : level_sets_dict.child_names())
    {
        retron::level_set_spec level_set_spec = ::load_level_set_spec(level_sets_dict.get<ff::dict>(name));
        spec.level_sets.try_emplace(std::string(name), std::move(level_set_spec));
    }

    for (std::string_view name : levels_dict.child_names())
    {
        retron::level_spec level_spec = ::load_level_spec(levels_dict.get<ff::dict>(name));
        spec.levels.try_emplace(std::string(name), std::move(level_spec));
    }

    return spec;
}
