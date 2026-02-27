#infdef MUD_BUFFER_H
#define MUD_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

typedef struct MudBuffer MudBuffer;

/* =============================================================================
 - Creation and Destruction
 -- `create`: Default initial capacity
 -- `create_with_capacity`: Pre-allocate for known size
 -- `create_from`: Init with copy of string
=============================================================================*/
MudBuffer* mud_buffer_create(void);
MudBuffer* mud_buffer_create_with_capacity(size_t capacity);
MudBuffer* mud_buffer_create_from(const char* str);
void mud_buffer_destroy(MudBuffer* buf);

/* =============================================================================
 - Size and Capacity
 -- `size` is content length, not including null terminator (matches strlen semantics)
=============================================================================*/
size_t mud_buffer_size(const MudBuffer* buf);
size_t mud_buffer_capacity(const MudBuffer* buf);
bool mud_buffer_is_empty(const MudBuffer* buf);
bool mud_buffer_reserve(MudBuffer* buf, size_t capacity);

/* =============================================================================
 - Content Access
 -- `cstr`: returns null-term string.  Valid until next mutation
 -- `data`: Mutable access to underlying bytes
 -- `char_at`: Single char access w/ bounds checking
=============================================================================*/
const char* mud_buffer_cstr(const MudBuffer* buf);
char* mud_buffer_data(MudBuffer* buf);
char mud_buffer_char_at(const MudBuffer* buf, size_t index);

/* =============================================================================
 - Appending
 -- `append_char`: Single char
 -- `append_str`: null-term string
 -- `append_bytes`: raw bytes (may contain nulls)
 -- `append_fmt`: print-style formatting
 -- `append_vfmt`: va_list version for wrapping in other variadic functions
=============================================================================*/
bool mud_buffer_append_char(MudBuffer* buf, char c);
bool mud_buffer_append_str(MudBuffer* buf, const char* str);
bool mud_buffer_append_bytes(MudBuffer* buf, const void* data, size_t len);
bool mud_buffer_append_fmt(MudBuffer* buf, const char* fmt, ...);
bool mud_buffer_append_vfmt(MudBuffer* buf, const char* fmt, va_list args);

/* =============================================================================
 - Modification
 -- `clear`: Reset to empty string
 -- `truncate`: shorten to given len
 -- `set_char`: Modify to single character
=============================================================================*/
void mud_buffer_clear(MudBuffer* buf);
bool mud_buffer_truncate(MudBuffer* buf, size_t new_size);
bool mud_buffer_set_char(MudBuffer* buf, size_t index, char c);

/* =============================================================================
 - Utility
 -- `clone`: Create independent copy
 -- `equals`: Compare two buffers
 -- `equals_str`: Compare buffer to C string
=============================================================================*/
MudBuffer* mud_buffer_clone(const MudBuffer* buf);
bool mud_buffer_euqals(const MudBuffer* a, const MudBuffer* b);
bool mud_buffer_equals_str(const MudBuffer* buf, const char* str);

#ifdef __cplusplus
}
#endif

#endif /* MUD_BUFFER_H */
