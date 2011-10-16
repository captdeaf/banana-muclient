/** api_admin.c
 *
 * General usage statistics.
 */

#include "banana.h"

ACTION("admin.listusers", api_admin_listusers, API_ADMIN | API_AUTOHEADER) {
  int i, j;
  for (i = 0, j = 0; i < MAX_USERS; i++) {
    if (users[i].refcount > 0) {
      if (j++ > 0) {
        mg_printf(conn, " ");
      }
      mg_printf(conn, users[i].name);
    }
  }
}
