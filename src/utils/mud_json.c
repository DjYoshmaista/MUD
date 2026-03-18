#include "mud_json.h"

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
    return cJSON_IsNumber(item) ? (int)item->valuedouble : def;
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
