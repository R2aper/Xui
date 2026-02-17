#include "utils.h"

#include <ctype.h>
#include <stddef.h>

const char *strcasestr_portable(const char *haystack, const char *needle) {
  if (!*needle) {
    return haystack;
  }
  for (; *haystack; ++haystack) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n &&
           tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
      h++;
      n++;
    }
    if (!*n) {
      return haystack;
    }
  }
  return NULL;
}
