#include "mud_hashmap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MUD_HASHMAP_INITIAL_CAPACITY 16
#define MUD_HASHMAP_LOAD_FACTOR_THRESHOLD 0.75

typedef enum {
    MUD_HASHMAP_ENTRY_EMPTY,
    MUD_HASHMAP_ENTRY_OCCUPIED,
    MUD_HASHMAP_ENTRY_TOMBSTONE
} MudHashmapEntryState;

typedef struct {
    char* key;
    void* value;
    MudHashmapEntryState state;
} MudHashmapEntry;

struct MudHashmap {
    MudHashmapEntry* entries;
    size_t capacity;
    size_t size;
};

static size_t hash_string(const char* key) {
    size_t hash = 14695981039346656037ULL;   // FNV offset basis

    while (*key) {
        hash ^= (unsigned char)*key;
        hash *= 1099511628211ULL;   // FNV prime
        key++;
    }

    return hash;
}

/* =============================================================================
 - Find Bucket
=============================================================================*/
static size_t find_bucket(const MudHashmapEntry* entries,
                          size_t capacity,
                          const char* key,
                          bool for_insert) {
    size_t hash = hash_string(key);
    size_t index = hash & (capacity - 1);  // Same as hash % capacity for power of 2 sizes
    size_t tombstone_index = SIZE_MAX;

    for (size_t i = 0; i < capacity; i++) {
        const MudHashmapEntry* entry = &entries[index];

        if (entry->state == MUD_HASHMAP_ENTRY_EMPTY) {
            if (for_insert && tombstone_index != SIZE_MAX) {
            return tombstone_index;
            }
            return index;
        }

        if (entry->state == MUD_HASHMAP_ENTRY_TOMBSTONE) {
            if (tombstone_index == SIZE_MAX) {
            tombstone_index = index;
            }
        } else if (strcmp(entry->key, key) == 0) {
            return index;
        }

        index = (index + 1) & (capacity -1);  // Linear probe
    }

    return tombstone_index != SIZE_MAX ? tombstone_index : SIZE_MAX;
}

MudHashmap* mud_hashmap_create(void) {
    return mud_hashmap_create_with_capacity(MUD_HASHMAP_INITIAL_CAPACITY);
}

MudHashmap* mud_hashmap_create_with_capacity(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = MUD_HASHMAP_INITIAL_CAPACITY;
    }

    // Round up to power of two
    size_t capacity = 1;
    while (capacity < initial_capacity) {
        capacity *= 2;
    }

    MudHashmap* map = malloc(sizeof(MudHashmap));
    if (map == NULL) {
        // TODO: Handle OOM
        return NULL;
    }

    map->entries = malloc(capacity * sizeof(MudHashmapEntry));
    if (map->entries == NULL) {
        free(map);
        return NULL;
    }

    map->capacity = capacity;
    map->size = 0;

    return map;
}

/* =============================================================================
 - Destruction
=============================================================================*/
void mud_hashmap_destroy(MudHashmap* map) {
    mud_hashmap_destroy_with(map, NULL);
}

void mud_hashmap_destroy_with(MudHashmap* map, MudHashmapDestructor destructor) {
    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        MudHashmapEntry* entry = &map->entries[i];
        if (entry->state == MUD_HASHMAP_ENTRY_OCCUPIED) {
            free(entry->key);
            if (destructor != NULL) {
                destructor(entry->value);
            }
        }
    }

    free(map->entries);
    free(map);
}

/* =============================================================================
 - Resize Helper
=============================================================================*/
static bool hashmap_resize(MudHashmap* map, size_t new_capacity) {
    MudHashmapEntry* new_entries = calloc(new_capacity, sizeof(MudHashmapEntry));
    if (new_entries == NULL) {
        return false;
    }

    // Rehash all existing entries
    for (size_t i = 0; i < map->capacity; i++) {
        MudHashmapEntry* old_entry = &map->entries[i];
        if (old_entry->state == MUD_HASHMAP_ENTRY_OCCUPIED) {
            size_t new_index = find_bucket(new_entries, new_capacity, old_entry->key, true);
            new_entries[new_index] = *old_entry;
        }
    }

    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;

    return true;
}

/* =============================================================================
 - Set (insert/update)
=============================================================================*/
bool mud_hashmap_set(MudHashmap* map, const char* key, void* value) {
    if (map == NULL || key == NULL) {
        return false;
    }

    // Check load factor
    if ((size_t)(map->size + 1) / map->capacity > MUD_HASHMAP_LOAD_FACTOR_THRESHOLD) {
        if (!hashmap_resize(map, map->capacity * 2)) {
            return false;
        }
    }

    size_t index = find_bucket(map->entries, map->capacity, key, true);
    if (index == SIZE_MAX) {
        return false;  // Should not happenw ith proper load factor
    }

    MudHashmapEntry* entry = &map->entries[index];

    // Update existing entry
    // TODO: Check for existing entry and return false if found
    //       (to avoid overwriting existing value)
    //       This would require a new flag in MudHashmapEntry
    //       to indicate whether the entry is occupied or not
    //       and a new flag to indicate whether the entry is
    //	     to be updated or not
    //       This would also require a new function to remove
    //	 an entry
    if (entry->state == MUD_HASHMAP_ENTRY_OCCUPIED) {
        entry->value = value;
    } else {
        // Insert new entry
        entry->key = strdup(key);
        if (entry->key == NULL) {
            return false;
        }
        entry->value = value;
        entry->state = MUD_HASHMAP_ENTRY_OCCUPIED;
        map->size++;
    }

    return true;
}

/* =============================================================================
 - Get and Has
=============================================================================*/
void* mud_hashmap_get(const MudHashmap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }

    size_t index = find_bucket(map->entries, map->capacity, key, false);
    if (index == SIZE_MAX) {
        return NULL;
    }

    MudHashmapEntry* entry = &map->entries[index];
    if (entry->state != MUD_HASHMAP_ENTRY_OCCUPIED) {
        return NULL;
    }

    return entry->value;
    // TODO: Return NULL if not found
    //       This would require a new flag in MudHashmapEntry
    //	     to indicate whether the entry is occupied or not
}

bool mud_hashmap_has(const MudHashmap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return false;
    }

    size_t index = find_bucket(map->entries, map->capacity, key, false);
    if (index == SIZE_MAX) {
        return false;
    }

    return map->entries[index].state == MUD_HASHMAP_ENTRY_OCCUPIED;
}

/* =============================================================================
 - Remove
=============================================================================*/
bool mud_hashmap_remove(MudHashmap* map, const char* key) {
    return mud_hashmap_remove_with(map, key, NULL);
}

bool mud_hashmap_remove_with(MudHashmap* map, const char* key, MudHashmapDestructor destructor) {
    if (map == NULL || key == NULL) {
        return false;
    }

    size_t index = find_bucket(map->entries, map->capacity, key, false);
    if (index == SIZE_MAX) {
        return false;
    }

    MudHashmapEntry* entry = &map->entries[index];
    if (entry->state != MUD_HASHMAP_ENTRY_OCCUPIED) {
        return false;
    }

    // TODO: Check for existing entry and return false if found
    //       (to avoid overwriting existing value)
    //       This would require a new flag in MudHashmapEntry
    //       to indicate whether the entry is occupied or not
    //       and a new flag to indicate whether the entry is
    //	     to be updated or not
    //       This would also require a new function to remove
    //	 an entry
    free(entry->key);
    entry->key = NULL;

    if (destructor != NULL) {
        destructor(entry->value);
    }
    entry->value = NULL;

    entry->state = MUD_HASHMAP_ENTRY_TOMBSTONE;
    map->size--;

    return true;
    // TODO: Return false if not found
}

/* =============================================================================
 - Size Operations
=============================================================================*/
size_t mud_hashmap_size(const MudHashmap* map) {
    return map ? map->size : 0;
}

bool mud_hashmap_is_empty(const MudHashmap* map) {
    return mud_hashmap_size(map) == 0;
    // TODO: Return false if not found
    //       This would require a new flag in MudHashmapEntry
    //	     to indicate whether the entry is occupied or not
}

void mud_hashmap_clear(MudHashmap* map) {
    mud_hashmap_clear_with(map, NULL);
}

void mud_hashmap_clear_with(MudHashmap* map, MudHashmapDestructor destructor) {
    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        MudHashmapEntry* entry = &map->entries[i];
        if (entry->state == MUD_HASHMAP_ENTRY_OCCUPIED) {
            free(entry->key);
            if (destructor != NULL) {
                destructor(entry->value);
            }
        }
        entry->key = NULL;
        entry->value = NULL;
        entry->state = MUD_HASHMAP_ENTRY_EMPTY;
    }

    map-> size = 0;
}

/* =============================================================================
 - Iteration
=============================================================================*/
MudHashmapIter mud_hashmap_iter_start(const MudHashmap* map) {
    (void)map;  // Unused, but API consistency
    return (MudHashmapIter){
        .key = NULL,
        .value = NULL,
        ._index = 0
    };
}

bool mud_hashmap_iter_next(const MudHashmap* map, MudHashmapIter* iter) {
    if (map == NULL || iter == NULL) {
        return false;
    }

    while (iter->_index < map->capacity) {
        MudHashmapEntry* entry = &map->entries[iter->_index];
        iter->_index++;

        if (entry->state == MUD_HASHMAP_ENTRY_OCCUPIED) {
            iter->key = entry->key;
            iter->value = entry->value;
            return true;
        }
    }

    return false;
}

/* =============================================================================
 - Keys Collection
=============================================================================*/
size_t mud_hashmap_keys(const MudHashmap* map, const char** out_keys, size_t max_keys) {
    if (map == NULL || out_keys == NULL || max_keys == 0) {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < map->capacity && count < max_keys; i++) {
        if (map->entries[i].state == MUD_HASHMAP_ENTRY_OCCUPIED) {
            out_keys[count++] = map->entries[i].key;
        }
    }

    return count;
}
