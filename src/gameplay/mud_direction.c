#include "mud_direction.h"

#include <ctype.h>
#include <stddef.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

static const char* k_dir_names[MUD_DIR_COUNT] = {
    "north",
    "south",
    "east",
    "west",
    "up",
    "down",
};

static const char* k_dir_short[MUD_DIR_COUNT] = {
    "n",
    "s",
    "e",
    "w",
    "u",
    "d",
};

_Static_assert(sizeof(k_dir_names) / sizeof(k_dir_names[0]) == MUD_DIR_COUNT, "k_dir_names must match MUD_DIR_COUNT");
_Static_assert(sizeof(k_dir_short) / sizeof(k_dir_short[0]) == MUD_DIR_COUNT, "k_dir_short must match MUD_DIR_COUNT");

static void trim_token(const char* input, const char** out_start, size_t* out_len) {
    const char* start = NULL;
    const char* end = NULL;

    if (out_start == NULL || out_len == NULL) {
        return;
    }

    *out_start = NULL;
    *out_len = 0;

    if (input == NULL) {
        return;
    }

    start = input;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    end = input + strlen(input);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }

    *out_start = start;
    *out_len = (size_t)(end - start);
}

static bool token_equals_ci(const char* token, size_t token_len, const char* literal) {
    size_t literal_len = 0;

    if (token == NULL || literal == NULL) {
        return false;
    }

    literal_len = strlen(literal);
    return token_len == literal_len && strncasecmp(token, literal, token_len) == 0;
}

bool mud_direction_is_valid(MudDirection dir) {
    const int value = (int)dir;
    return value >= 0 && value < (int)MUD_DIR_COUNT;
}

size_t mud_direction_count(void) {
    return (size_t)MUD_DIR_COUNT;
}

const char* mud_direction_name(MudDirection dir) {
    if (!mud_direction_is_valid(dir)) {
        return "unknown";
    }

    return k_dir_names[(size_t)dir];
}

const char* mud_direction_short(MudDirection dir) {
    if (!mud_direction_is_valid(dir)) {
        return "?";
    }

    return k_dir_short[(size_t)dir];
}

bool mud_direction_parse(const char* token, MudDirection* out_dir) {
    const char* start = NULL;
    size_t len = 0;
    size_t i = 0;

    if (out_dir == NULL) {
        return false;
    }

    *out_dir = MUD_DIR_COUNT;

    trim_token(token, &start, &len);
    if (start == NULL || len == 0) {
        return false;
    }

    for (i = 0; i < (size_t)MUD_DIR_COUNT; i++) {
        if (token_equals_ci(start, len, k_dir_names[i]) || token_equals_ci(start, len, k_dir_short[i])) {
            *out_dir = (MudDirection)i;
            return true;
        }
    }

    return false;
}

bool mud_direction_opposite(MudDirection dir, MudDirection* out_opposite) {
    if (out_opposite == NULL || mud_direction_is_valid(dir)) {
        if (out_opposite != NULL) {
            *out_opposite = MUD_DIR_COUNT;
        }
        return false;
    }

    switch(dir) {
        case MUD_DIR_NORTH:
            *out_opposite = MUD_DIR_SOUTH;
            return true;
        case MUD_DIR_SOUTH:
            *out_opposite = MUD_DIR_NORTH;
            return true;
        case MUD_DIR_EAST:
            *out_opposite = MUD_DIR_WEST;
            return true;
        case MUD_DIR_WEST:
            *out_opposite = MUD_DIR_EAST;
            return true;
        case MUD_DIR_UP:
            *out_opposite = MUD_DIR_DOWN;
            return true;
        case MUD_DIR_DOWN:
            *out_opposite = MUD_DIR_UP;
            return true;
        case MUD_DIR_COUNT:
            break;
        case MUD_DIR_INVALID:
            break;
    }

    *out_opposite = MUD_DIR_COUNT;
    return false;
}

int mud_direction_to_db(MudDirection dir) {
    if (!mud_direction_is_valid(dir)) {
        return -1;
    }
    return (int)dir;
}

bool mud_direction_from_db(int value, MudDirection* out_dir) {
    if (out_dir == NULL) {
        return false;
    }

    if (value < 0 || value >= (int)MUD_DIR_COUNT) {
        *out_dir = MUD_DIR_COUNT;
        return false;
    }

    *out_dir = (MudDirection)value;
    return true;
}
