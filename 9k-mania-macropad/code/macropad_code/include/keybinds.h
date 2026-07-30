#pragma once

#include <Arduino.h>

extern const int total_keys = 10;

enum class CommandType {
    Key,
    Text,
    None
};

struct KeyCommand {
    CommandType type;
    uint16_t key;
    const char *text;
};

extern const KeyCommand switchKeysSetOne[total_keys] = {
    { CommandType::Key, 'a', nullptr },
    { CommandType::Key, 's', nullptr },
    { CommandType::Key, 'd', nullptr },
    { CommandType::Key, 'f', nullptr },
    { CommandType::Key, ' ', nullptr },
    { CommandType::Key, ' ', nullptr },
    { CommandType::Key, 'j', nullptr },
    { CommandType::Key, 'k', nullptr },
    { CommandType::Key, 'l', nullptr },
    { CommandType::Key, ';', nullptr },
};

extern const KeyCommand switchKeysSetTwo[total_keys] = {
    { CommandType::Key, KEY_ESC, nullptr },
    { CommandType::Key, KEY_F3, nullptr },
    { CommandType::Key, KEY_F4, nullptr },
    { CommandType::Text, '\0', "mode=mania key=4"},
    { CommandType::Text, '\0', "mode=mania key=7"},
    { CommandType::Text, '\0', "mode=osu"},
    { CommandType::None, '\0', nullptr },
    { CommandType::None, '\0', nullptr },
    { CommandType::None, '\0', nullptr },
    { CommandType::None, '\0', nullptr }
};