#include <string.h>
#include "bn_mem.h"

#define DEFAULT_SEPARATOR ":"
#define CH_ZERO '\0'

char* bn_strdup(const char *str, const char *file, int line)
{
    char *ret;
    size_t len;

    if (str == NULL) return NULL;

    len = strlen(str) + 1;
    ret = mm_malloc(len, file, line);
    if (ret == NULL) return NULL;

    memcpy(ret, str, len);
    return ret;
}