// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <string>
#include <string.h>
#include <unistd.h>

#include "utils.hpp"
#include "clustering/administration/main/names.hpp"

const char* names[] = {
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

std::string get_random_machine_name() {
    int index = randint(sizeof(names) / sizeof(char *));
    return std::string(names[index]);
}

bool is_invalid_char(char ch) {
    return ! (isalpha(ch) || isdigit(ch) || ch == '_');
}

std::string get_machine_name(const int port_offset) {
    char h[64];

    h[0] = '\0';
    if (gethostname(h, sizeof(h) - 1) < 0 || strlen(h) == 0) {
        return get_random_machine_name();
    }
    h[sizeof(h) - 1] = '\0';

    std::string sanitized(h);
    std::replace_if(sanitized.begin(), sanitized.end(), is_invalid_char, '_');
    if (isdigit(sanitized[0])) {
        sanitized[0] = '_';
    }

    if (port_offset > 0) {
        std::stringstream ss;
        ss << port_offset;
        if (sanitized[sanitized.size() - 1] != '_') {
            sanitized += '_';
        }
        sanitized += ss.str();
    }
    return sanitized;
}
