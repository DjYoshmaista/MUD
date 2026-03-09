#include "mud_utils.h"

#include <stdlib.h>
#include <string.h>

char* mud_strdup(const char* src) {
    if (src == NULL) {
        return NULL;
    }

    size_t duplength = strlen(src) + 1;

    char* copy = malloc(duplength);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, src, duplength);

    return copy;
}
