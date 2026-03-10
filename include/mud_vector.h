#ifndef MUD_VECTOR_H
#define MUD_VECTOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MudVector MudVector;

/* Creation & Destruction */
MudVector* mud_vector_create(size_t element_size);
void mud_vector_destroy(MudVector* vec);

/* Capacity Management */
size_t mud_vector_size(const MudVector* vec);
size_t mud_vector_capacity(const MudVector* vec);
bool mud_vector_is_empty(const MudVector* vec);

/* Reserve and Shrink */
bool mud_vector_reserve(MudVector* vec, size_t capacity);
bool mud_vector_shrink_to_fit(MudVector* vec);

/* Element Access */
void* mud_vector_get(const MudVector* vec, size_t index);
void* mud_vector_get_unchecked(const MudVector* vec, size_t index);
bool mud_vector_set(MudVector* vec, size_t index, const void* element);

/* Adding Elements */
bool mud_vector_push(MudVector* vec, const void* element);
bool mud_vector_insert(MudVector* vec, size_t index, const void* element);

/* Removing Elements */
bool mud_vector_pop(MudVector* vec, void* out_element);
bool mud_vector_remove(MudVector* vec, size_t index);
void mud_vector_clear(MudVector* vec);

/* Bulk Operations */
void* mud_vector_data(MudVector* vec);
const void* mud_vector_data_const(const MudVector* vec);

/* Type-Safe Wrapper Macros */
#define MUD_VECTOR_PUSH(vec, type, value)       \
    do {                                        \
        type _tmp = (value);                        \
        mud_vector_push((vec), &_tmp);              \
    } while (0)                                 \

#define MUD_VECTOR_GET(vec, type, index)        \
    (*(type*)mud_vector_get((vec), (index)))    \

#define MUD_VECTOR_GET_PTR(vec, type, index)    \
    ((type*)mud_vector_get((vec), (index)))     \

#define MUD_VECTOR_SET(vec, type, index, value) \
    do {                                        \
        type _tmp = (value);                        \
        mud_vector_set((vec), (index), &_tmp);      \
    } while (0)                                 \

#define MUD_VECTOR_INSERT(vec, type, index, value) \
    do {                                            \
        type _tmp = (value);                            \
        mud_vector_insert((vec), (index), &_tmp);       \
    } while (0)                                     \

#define MUD_VECTOR_REMOVE(vec, type, index)      \
    do {                                          \
        mud_vector_remove((vec), (index));            \
    } while (0)                                   \

#ifdef __cplusplus
}
#endif

#endif /* MUD_VECTOR_H */
