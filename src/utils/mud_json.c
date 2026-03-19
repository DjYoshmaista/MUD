#include "mud_json.h"
#include <limits.h>
#include <math.h>

cJSON* mud_json_parse (const char* s) {
    return s ? cJSON_Parse(s) : NULL;
}

char* mud_json_print(const cJSON* root) {
    return root ? cJSON_Print(root) : NULL;
}

char* mud_json_print_compact(const cJSON* root) {
    return root ? cJSON_PrintUnformatted(root) : NULL;
}

const char* mud_json_get_string(const cJSON* obj, const char* key, const char* def) {
    if (!obj || !key) return def;
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(item) || !item->valuestring) return def;
    return item->valuestring;
}

int mud_json_get_int(const cJSON* obj, const char* key, int def) {
    if (!obj || !key) return def;
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsNumber(item)) return def;

    double value = item->valuedouble;
    if (value < (double)INT_MIN || value > (double)INT_MAX) return def;
    if (trunc(value) != value) return def;

    return (int)value;
}

double mud_json_get_double(const cJSON* obj, const char* key, double def) {
    if (!obj || !key) return def;
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(item) ? item->valuedouble : def;
}

bool mud_json_get_bool(const cJSON* obj, const char* key, bool def) {
    if (!obj || !key) return def;
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsTrue(item)) return true;
    if (cJSON_IsFalse(item)) return false;
    return def;
}
