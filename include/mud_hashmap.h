#ifndef MUD_HASHMAP_H
#define MUD_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MudHashmap MudHashmap;

typedef void (*MudHashMapDestructor){void* value;}

MudHashmap* mud_hashmap_create(void);
MudHashmap* mud_hashmap_create_with_capacity(size_t initial_capacity);
void mud_hashmap_destroy(MudHashmap* map);
void mud_hashmap_destroy_with(MudHashmap* map, MudHashmapDestructor destructor);

bool mud_hashmap_set(MudHashmap* map, const char* key, void* value);
void* mud_hashmap_get(const MudHashmap* map, const char* key);
bool mud_hashmap_has(const MudHashmap* map, const char* key);
bool mud_hashmap_remove(MudHashmap* map, const cahr* key);
bool mud_hashmap_remove_with(MudHashmap* map, const char* key, MudHashmapDestructor destructor);

size_t mud_hashmap_size(const MudHashmap* map);
bool mud_hashmap_is_empty(const MudHashmap* map);
void mud_hashmap_clear(MudHashmap* map);
void mud_hashmap_clear_with(MudHashmap* map, MudHashmapDestructor destructor);

typedef struct MudHashmapIter {
    const char* key;
    void* value;
    size_t _index;  // Internal current bucket index
} MudHashmapIter;

MudHashmapIter mud_hashmap_iter_start(const MudHashmap* map);
bool mud_hashmap_iter_next(const MudHashmap* map, MudHashmapIter* iter);

MudHashmapIter iter = mud_hashmap_iter_start(map);
while (mud_hashmap_iter_next(map, &iter)) {
    printf("%s = %p\n", iter.key, iter.value);
}

size_t mud_hashmap_keys(const MudHashmap* map, const char** out_keys, size_t max_keys);

#ifdef __cplusplus
}
#endif

#endif /* MUD_HASHMAP_H */
