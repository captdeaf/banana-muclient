/** api_admin.c
 *
 * General usage statistics.
 */

#include "banana.h"

ACTION("admin.listusers", api_admin_listusers, API_ADMIN | API_AUTOHEADER) {
  int i, j;
  char retbuff[8192];
  int len = 8192;
  char *ptr = retbuff;
  for (i = 0, j = 0; i < MAX_USERS; i++) {
    if (users[i].refcount > 0) {
      if (j++ > 0) {
        ptr += snprintf(ptr, len, " %s", users[i].name);
      } else {
        ptr += snprintf(ptr, len, "%s", users[i].name);
      }
      len = 8192 - (ptr - retbuff);
    }
  }
  mg_printf(conn, "%s", retbuff);
}
