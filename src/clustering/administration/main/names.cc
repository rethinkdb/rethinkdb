// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/names.hpp"

#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <algorithm>

#include "random.hpp"
#include "utils.hpp"

static const char* names[] = {
    "Akasha",
    "Alchemist",
    "Azwraith",
    "Bane",
    "Batrider",
    "Beastmaster",
    "Bradwarden",
    "Brewmaster",
    "Bristleback",
    "Chaosknight",
    "Chen",
    "Clinkz",
    "Clockwerk",
    "Courier",
    "Dazzle",
    "Dragonknight",
    "Dragonus",
    "Drow",
    "Earthshaker",
    "Enchantress",
    "Enigma",
    "Ezalor",
    "Furion",
    "Gondar",
    "Gyrocopter",
    "Huskar",
    "Invoker",
    "Jakiro",
    "Juggernaut",
    "Kaldr",
    "Krobelus",
    "Kunkka",
    "Lanaya",
    "LeDirge",
    "Leshrac",
    "Lina",
    "Lion",
    "Luna",
    "Lycanthrope",
    "Magnus",
    "Medusa",
    "Meepo",
    "Mirana",
    "Morphling",
    "Mortred",
    "Naix",
    "Nevermore",
    "NyxNyxNyx",
    "Ogre",
    "Omniknight",
    "Ostarion",
    "Outworld",
    "Phoenix",
    "Puck",
    "Pudge",
    "Pugna",
    "Razor",
    "Rhasta",
    "Rikimaru",
    "Roshan",
    "Rubick",
    "Rylai",
    "Sandking",
    "Silencer",
    "Slardar",
    "Slark",
    "Slithice",
    "Spectre",
    "Stormspirit",
    "Strygwyr",
    "Sven",
    "Sylla",
    "Tidehunter",
    "Tinker",
    "Tiny",
    "Treant",
    "Tusk",
    "Ursa",
    "Viper",
    "Visage",
    "Weaver",
    "Windrunner",
    "Wisp",
    "Worldsmith"
};

name_string_t get_random_server_name() {
    int index = randint(sizeof(names) / sizeof(char *));
    return name_string_t::guarantee_valid(names[index]);
}

bool is_invalid_char(char ch) {
    return !(isalpha(ch) || isdigit(ch) || ch == '_');
}

char rand_alnum() {
    int rand_val = randint(10 + 26);
    return (rand_val < 10) ? ('0' + rand_val) : ('a' + rand_val - 10);
}

name_string_t get_default_server_name() {
    char h[64];
    h[sizeof(h) - 1] = '\0';

    if (gethostname(h, sizeof(h) - 1) != 0 || strlen(h) == 0) {
        return get_random_server_name();
    }

    std::string sanitized(h);
    std::replace_if(sanitized.begin(), sanitized.end(), is_invalid_char, '_');

    return name_string_t::guarantee_valid(
        strprintf("%s_%c%c%c",
            sanitized.c_str(),
            rand_alnum(), rand_alnum(), rand_alnum()).c_str());
}
