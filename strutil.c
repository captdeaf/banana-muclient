/* strutil.c
 *
 * String utils.
 */

#include "banana.h"

void
strchomp(char *str) {
  char *ptr = str + strlen(str) - 1;
  while (ptr && ptr >= str && isspace(*ptr)) {
    *(ptr--) = '\0';
  }
}
