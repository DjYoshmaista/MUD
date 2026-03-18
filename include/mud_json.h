#ifndef MUD_JSON_H
#define MUD_JSON_H

#include <stdbool.h>
#include <stddef.h>

// Expose cJSON directly - tthe API is stable and small enough that wrapping it adds no clarity
#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Conveience: parse JSON from a null-terminated string.  Returns NULL on error, caller owns result and must call cJSON_Delete() on it when finished. */
cJSON* mud_json_parse(const char* json_str);

/*  Convenience: serialize a cJSON tree to a newly allocated string.
    Returns NULL on failure.  Caller must call cJSON_free() on the result.
    (Not free() - cJSON's allocator may differ from the system one. */
char* mud_json_print(const cJSON* root);

/*  Convenience: serialize without whitespace (compact for network transmission)
    Caller must cJSON_free() the result */
char* mud_json_print_compact(const cJSON* root);

/*  Safe field extraction helpers
    These return the value or a default - they never crash on NULL or wrong types
*/
const char* mud_json_get_string(const cJSON* obj, const char* key, const char* default_val);
int         mud_juson_get_int(const cJSON* obj, const char* key, int default_val);
double      mud_json_get_double(const cJSON* obj, const char* key, double default_val);
bool        mud_json_get_bool(const cJSON* obj, const char* key, bool default_val);

#ifdef __cplusplus
}
#endif

#endif // MUD_JSON_H
