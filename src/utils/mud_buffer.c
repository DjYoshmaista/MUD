#include "mud_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MUD_BUFFER_INITIAL_CAPACITY 128

struct MudBuffer {
    char* data;
    size_t size;
    size_t capacity;
};

/* =============================================================================
 - Creation
 -- Capacity +1: Always alloc space for null term beyond stated capacity
 -- Init to empty string: `data[0] = '\0'` makes buf immediately usable as cstr
=============================================================================*/
MudBuffer* mud_buffer_create(void) {
    return mud_buffer_create_with_capacity(MUD_BUFFER_INITIAL_CAPACITY);
}

MudBuffer* mud_buffer_create_with_capacity(size_t capacity) {
    if (capacity == 0) {
        capacity = MUD_BUFFER_INITIAL_CAPACITY;
    }

    MudBuffer* buf = malloc(sizeof(MudBuffer));
    if (buf == NULL) {
        return NULL;
    }

    // +1 for null terminator
    buf->data = malloc(capacity + 1);
    if (buf->data == NULL) {
        free(buf);
        return NULL;
    }

    buf->data[0] = '\0';
    buf->size = 0;
    buf->capacity = capacity;

    return buf;
}

MudBuffer* mud_buffer_create_from(const char* fromStr) {
    size_t initial = fromStr ? strlen(fromStr) : 0;
    MudBuffer* buf = mud_buffer_create_with_capacity(initial);
    if (buf == NULL) return NULL;
    if (fromStr != NULL && !mud_buffer_append_str(buf, fromStr)) {
        mud_buffer_destroy(buf);
        return NULL;
    }
    return buf;
}

/* =============================================================================
 - Destruction and Accessors
=============================================================================*/
void mud_buffer_destroy(MudBuffer* buf) {
    if (buf == NULL) {
        return;
    }
    free(buf->data);
    free(buf);
}

size_t mud_buffer_size(const MudBuffer* buf) {
    return buf ? buf->size : 0;
}

size_t mud_buffer_capacity(const MudBuffer* buf) {
    return buf ? buf->capacity : 0;
}

bool mud_buffer_is_empty(const MudBuffer* buf) {
    return mud_buffer_size(buf) == 0;
}

const char* mud_buffer_cstr(const MudBuffer* buf) {
    if (buf == NULL) {
        return "";
    }
    return buf->data;
}

char* mud_buffer_data(MudBuffer* buf) {
    return buf ? buf->data : NULL;
}

char mud_buffer_char_at(const MudBuffer* buf, size_t index) {
    if (buf == NULL || index >= buf->size) {
        return '\0';
    }
    return buf->data[index];
}

/* =============================================================================
 - Growth Helper
=============================================================================*/
static bool buffer_ensure_capacity(MudBuffer* buf, size_t needed) {
    if (needed <= buf->capacity) {
        return true;
    }

    size_t new_capacity = buf->capacity;
    while (new_capacity < needed) {
        if (new_capacity > (SIZE_MAX / 2U)) {
            return false;
        }
        new_capacity *= 2;
    }

    if (new_capacity == SIZE_MAX) {
        return false;
    }

    char* new_data = realloc(buf->data, new_capacity + 1);
    if (new_data == NULL) {
        return false;
    }

    buf->data = new_data;
    buf->capacity = new_capacity;
    return true;
}

bool mud_buffer_reserve(MudBuffer* buf, size_t capacity) {
    return buf != NULL && buffer_ensure_capacity(buf, capacity);
}

/* =============================================================================
 - Append Operations
=============================================================================*/
bool mud_buffer_append_char(MudBuffer* buf, char c) {
    if (buf == NULL) {
        return false;
    }

    if (buf->size == SIZE_MAX) {
        return false;
    }

    if (!buffer_ensure_capacity(buf, buf->size + 1)) {
        return false;
    }

    buf->data[buf->size] = c;
    buf->size++;
    buf->data[buf->size] = '\0';
    return true;
}

bool mud_buffer_append_str(MudBuffer* buf, const char* str) {
    if (buf == NULL || str == NULL) {
        return false;
    }

    size_t len = strlen(str);
    if (len == 0) {
        return true;
    }

    if (len > SIZE_MAX - buf->size) {
        return false;
    }

    if (!buffer_ensure_capacity(buf, buf->size + len)) {
        return false;
    }

    memcpy(buf->data + buf->size, str, len);
    buf->size += len;
    buf->data[buf->size] = '\0';

    return true;
}

bool mud_buffer_append_bytes(MudBuffer* buf, const void* data, size_t len) {
    if (buf == NULL || (data == NULL && len > 0)) {
        return false;
    }

    if (len == 0) {
        return true;
    }

    if (len > SIZE_MAX - buf->size) {
        return false;
    }

    if (!buffer_ensure_capacity(buf, buf->size + len)) {
        return false;
    }

    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    buf->data[buf->size] = '\0';

    return true;
}

/* =============================================================================
 - Formatted Append
=============================================================================*/
bool mud_buffer_append_vfmt(MudBuffer* buf, const char* fmt, va_list args) {
    if (buf == NULL || fmt == NULL) {
        return false;
    }

    va_list args_copy;
    va_copy(args_copy, args);

    // Determine required size
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        return false;  // Format error
    }

    if ((size_t)needed > SIZE_MAX - buf->size) {
        return false;
    }

    if (!buffer_ensure_capacity(buf, buf->size + (size_t)needed)) {
        return false;
    }

    vsnprintf(buf->data + buf->size, (size_t)needed + 1, fmt, args);
    buf->size += (size_t)needed;

    return true;
}

bool mud_buffer_append_fmt(MudBuffer* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bool result = mud_buffer_append_vfmt(buf, fmt, args);
    va_end(args);
    return result;
}

/* =============================================================================
 - Modification Operations
=============================================================================*/
void mud_buffer_clear(MudBuffer* buf) {
    if (buf == NULL) {
        return;
    }
    buf->size = 0;
    buf->data[0] = '\0';
}

bool mud_buffer_truncate(MudBuffer* buf, size_t new_size) {
    if (buf == NULL) {
        return false;
    }

    if (new_size >= buf->size) {
        return true;  // Nothing to do
    }

    buf->size = new_size;
    buf->data[new_size] = '\0';

    return true;
}

bool mud_buffer_set_char(MudBuffer* buf, size_t index, char c) {
    if (buf == NULL || index >= buf->size) {
        return false;
    }
    buf->data[index] = c;
    return true;
}

/* =============================================================================
 - Clone and Comparison
=============================================================================*/
MudBuffer* mud_buffer_clone(const MudBuffer* buf) {
    if (buf == NULL) {
        return NULL;
    }

    MudBuffer* clone = mud_buffer_create_with_capacity(buf->size);
    if (clone == NULL) {
        return NULL;
    }

    memcpy(clone->data, buf->data, buf->size + 1);
    clone->size = buf->size;
    return clone;
}

bool mud_buffer_equals(const MudBuffer* a, const MudBuffer* b) {
    if (a == b) {
        return true;  // Same pointer or both NULL
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->size != b->size) {
        return false;
    }
    return memcmp(a->data, b->data, a->size) == 0;
}

bool mud_buffer_equals_str(const MudBuffer* buf, const char* str) {
    if (buf == NULL && str == NULL) {
        return true;
    }
    if (buf == NULL || str == NULL) {
        return false;
    }
    return strcmp(buf->data, str) == 0;
}
