#include "pch.h"
#include "source/core/options.h"

NS_IMPLEMENT_REFLECTION_ENUM(retron::game_flags, "retron.game_type")
{
    NsVal("normal", retron::game_flags::normal);
    NsVal("infinite_lives", retron::game_flags::infinite_lives);
    NsVal("no_bosses", retron::game_flags::no_bosses);
}

NS_IMPLEMENT_REFLECTION_ENUM(retron::game_players, "retron.game_players")
{
    NsVal("one", retron::game_players::one);
    NsVal("two_coop", retron::game_players::two_coop);
    NsVal("two_take_turns", retron::game_players::two_take_turns);
}

NS_IMPLEMENT_REFLECTION_ENUM(retron::game_difficulty, "retron.game_difficulty")
{
    NsVal("baby", retron::game_difficulty::baby);
    NsVal("easy", retron::game_difficulty::easy);
    NsVal("normal", retron::game_difficulty::normal);
    NsVal("hard", retron::game_difficulty::hard);
}

retron::game_options::game_options()
    : version(retron::game_options::CURRENT_VERSION)
    , flags(game_flags::default)
    , players(game_players::default)
    , difficulty(game_difficulty::default)
{}

std::string_view retron::game_options::difficulty_id() const
{
    switch (this->difficulty)
    {
        case retron::game_difficulty::baby:
            return "baby";

        case retron::game_difficulty::easy:
            return "easy";

        default:
        case retron::game_difficulty::normal:
            return "normal";

        case retron::game_difficulty::hard:
            return "hard";
    }
}

size_t retron::game_options::player_count() const
{
    switch (this->players)
    {
        default:
            return 1;

        case game_players::two_take_turns:
        case game_players::two_coop:
            return 2;
    }
}

bool retron::game_options::coop() const
{
    return this->players == retron::game_players::two_coop;
}

retron::system_options::system_options()
    : version(retron::system_options::CURRENT_VERSION)
    , full_screen(false)
    , sound(true)
    , music(true)
    , sound_volume(1)
    , music_volume(1)
{}
