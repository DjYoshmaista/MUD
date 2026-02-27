#include "mud_vector.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct MudVector {
    void* data;
    size_t size;
    size_t capacity;
    size_t element_size;
};

/* Constants: Inital Capacity & Growth Factor */
#define MUD_VECTOR_INITIAL_CAPACITY 8
#define MUD_VECTOR_GROWTH_FACTOR 2

/* Helper: Calculate Byte Offset */
static inline void* vector_element_ptr(const MudVector* vec, size_t index) {
    return (char*)vec->data + (index * vec->element_size);
}

/* =============================================================================
 * Creation->Process
 * - Validate element_size (must be > 0)
 * - Allocate the struct itself
 * - Check for allocation failure
 * - Allocate the data array
 * - Check for allocation failure, clean up if so
 * - Initialize fields
 * - Return
 ============================================================================= */
MudVector* mud_vector_create(size_t element_size) {
    if (element_size == 0) {
	return NULL;
    }

    MudVector* vec = malloc(sizeof(MudVector));
    if (vec == NULL) {
	return NULL;
    }

    vec->data = malloc(element_size * MUD_VECTOR_INITIAL_CAPACITY);
    if (vec->data == NULL) {
	free(vec);
	return NULL;
    }

    vec->size = 0;
    vec->capacity = MUD_VECTOR_INITIAL_CAPACITY;
    vec->element_size = element_size;

    return vec;
}

/* =============================================================================
 * Destruction->Process
 * - Null check: safe to call with NULL; matches free() behavior, simplifies caller code
 * - Free data first, then struct.  Reversing would access free memory
 * ============================================================================= */
void mud_vector_destroy(MudVector* vec) {
    if (vec == NULL) {
	return;
    }

    free(vec->data);
    free(vec);
}

/* =============================================================================
 * Size Accessors: Defensive NULL checks
 ============================================================================= */
size_t mud_vector_size(const MudVector* vec) {
    if (vec == NULL) {
	return 0;
    }
    return vec->size;
}

size_t mud_vector_capacity(const MudVector* vec) {
    if (vec == NULL) {
	return 0;
    }
    return vec->capacity;
}

bool mud_vector_is_empty(const MudVector* vec) {
    return mud_vector_size(vec) == 0;
}

/* =============================================================================
 * Growth Helper: realloc behavior --
 *   - If successful, returns pointer to resized block (may be same or diff addr);
 *     copies existing data.
 * - If failed; Returns NULL, original block unchanged
 * ============================================================================= */
static bool vector_grow(MudVector* vec) {
    size_t new_capacity = vec->capacity * MUD_VECTOR_GROWTH_FACTOR;

    void* new_data = realloc(vec->data, new_capacity * vec->element_size);
    if (new_data == NULL) {
        return false;
    }

    vec->data = new_data;
    vec->capacity = new_capacity;
    return true;
}

/* =============================================================================
* Reserve:
* - Idempotent: Calling with smaller capacity than current is a no-op success
* - Exact Allocation: Unlike growth (which doubles), reserve allocates exactly
*   what's requested
* ============================================================================ */
bool mud_vector_reserve(MudVector* vec, size_t min_capacity) {
    if (vec == NULL) {
	return false;
    }

    if (min_capacity <= vec->capacity) {
	return true; // Already have enough
    }

    void* new_data = realloc(vec->data, min_capacity * vec->element_size);
    if (new_data == NULL) {
	return false;
    }

    vec->data = new_data;
    vec->capacity = min_capacity;
    return true;
}

/* =============================================================================
 * Shrink to Fit:
 * - Empty vector special case: don't shrink to 0 capacity (would need special
 *   handling everywhere).  Shrink to initial capacity instead.
 * - Failure bahvior: If realloc fails, keep existing (oversized) allocation.
 *   Vector still works.
 * ============================================================================= */
bool mud_vector_shrink_to_fit(MudVector* vec) {
    if (vec == NULL) {
	return false;
    }

    if (vec->size == vec->capacity) {
	return true; // Already minimal
    }

    if (vec->size == 0) {
	free(vec->data);
	vec->data = malloc(vec->element_size * MUD_VECTOR_INITIAL_CAPACITY);
	if (vec->data == NULL) {
	    vec->capacity = 0;
	    return false;
	}
	vec->capacity = MUD_VECTOR_INITIAL_CAPACITY;
	return true;
    }

    void* new_data = realloc(vec->data, vec->size * vec->element_size);
    if (new_data == NULL) {
	return false; // Keep oversized allocation
    }

    vec->data = new_data;
    vec->capacity = vec->size;
    return true;
}

/* =============================================================================
*  Element Access
*  - get vs get_unchecked: Checked ver validates bounds.  Unchecked assumes valid
*  set copies uses memcpy to copy element_size bytes from src to dst
*  ============================================================================= */
void* mud_vector_get(const MudVector* vec, size_t index) {
    if (vec == NULL || index >= vec->size) {
	return NULL;
    }
    return vector_element_ptr(vec, index);
}

void* mud_vector_get_unchecked(const MudVector* vec, size_t index) {
    return vector_element_ptr(vec, index);
}

bool mud_vector_set(MudVector* vec, size_t index, const void* element) {
    if (vec == NULL || index >= vec->size || element == NULL) {
	return false;
    }

    memcpy(vector_element_ptr(vec, index), element, vec->element_size);
    return true;
}

/* =============================================================================
*  Push: Logic Flow
*  - Validate inputs
*  - Grow if at capacity
*  - Copy element to end
*  - Increment size
*  ============================================================================= */
bool mud_vector_push(MudVector* vec, const void* element) {
    if (vec == NULL || element == NULL) {
	return false;
    }

    if (vec->size >= vec->capacity) {
	if (!vector_grow(vec)) {
	    return false;
	}
    }

    memcpy(vector_element_ptr(vec, vec->size), element, vec->element_size);
    vec->size++;
    return true;
}

/* =============================================================================
*  Insert
*  - index > vec->size: check index=size means append (valid)  index>size invalid
*  - Delegate to push (inserting at end is pushing) - reuse code.
*  - memmove vs memcpy:
*    - memmove handles overlapping regions; source and destination overlap
*      (shifting elements within same array)
*    - memcpy would be undefined behavior
*  ============================================================================= */
bool mud_vector_insert(MudVector* vec, size_t index, const void* element) {
    if (vec == NULL || element == NULL || index > vec->size) {
	return false;
    }

    if (index == vec->size) {
	return mud_vector_push(vec, element);
    }

    if (vec->size >= vec->capacity) {
	if (!vector_grow(vec)) {
	    return false;
	}
    }

    // Shift elements right
    memmove(
	vector_element_ptr(vec, index + 1),
	vector_element_ptr(vec, index),
	(vec->size - index) * vec->element_size
    );

    memcpy(vector_element_ptr(vec, index), element, vec->element_size);
    vec->size++;
    return true;
}

/* =============================================================================
*  Pop
*  - Decrement size, making last element "not part of vector" (still in memory)
*  - If caller provides buffer, copy removed element there.  Otherwise discard.
*  ============================================================================= */
bool mud_vector_pop(MudVector* vec, void* out_element) {
    if (vec == NULL || vec->size == 0) {
	return false;
    }

    vec->size--;

    if (out_element != NULL) {
	memcpy(out_element, vector_element_ptr(vec, vec->size), vec->element_size);
    }

    return true;
}

/* =============================================================================
*  Remove
*  - Removing last element: Just decrement size, no shifting necessary
*  - Shift elements after the removed element, one position left
*  ============================================================================= */
bool mud_vector_remove(MudVector* vec, size_t index) {
    if (vec == NULL || index >= vec->size) {
	return false;
    }

    if (index == vec->size - 1) {
	vec->size--;
	return true;
    }

    // Shift elements left
    memmove(
	vector_element_ptr(vec, index),
	vector_element_ptr(vec, index + 1),
	(vec->size - index - 1) * vec->element_size
    );

    vec->size--;
    return true;
}

/* =============================================================================
*  Clear
*  - Just reset size.  Memory remmains allocated.  Capacity unchanged
*  ============================================================================= */
void mud_vector_clear(MudVector* vec) {
    if (vec == NULL) {
	return;
    }
    vec->size = 0;
}

/* =============================================================================
*  Data Access
*  - Two versions: Mutable and const.  Allows compiler to enforce const-correctness
*  ============================================================================= */
void* mud_vector_data(MudVector* vec) {
    if (vec == NULL) {
	return NULL;
    }
    return vec->data;
}

const void* mud_vector_data_const(const MudVector* vec) {
    if (vec == NULL) {
	return NULL;
    }
    return vec->data;
}
