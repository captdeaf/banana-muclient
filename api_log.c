/** api_file.c
 *
 * /action/file.foo api calls.
 */

#include "banana.h"

int
valid_log_name(char *fname) {
  int len = strlen(fname);
  // u-w-YYYY-MM-DD.log
  if (len < 18) return 0;
  if (strcmp(fname + len - 4, ".log")) {
    return 0;
  }
  if (!isalnum(*fname)) return 0;
  for (;*fname;fname++) {
    if (isalpha(*fname)) continue;
    if (isdigit(*fname)) continue;
    if (*fname == '-') continue;
    if (*fname == '_') continue;
    if (*fname == '.') continue;
  }
  return 1;
}

static char *
user_log_path(User *user, char *fname) {
  char path[100];
  if (!valid_log_name(fname)) return NULL;
  snprintf(path, 100, "%s/logs", user->dir);
  file_mkdir(path);
  snprintf(path, 100, "%s/logs/%s", user->dir, fname);
  return strdup(path);
}

void
list_user_logs(User *user,
                struct mg_connection *conn,
                struct mg_request_info *req _unused_) {
  char path[100];
  snprintf(path, 100, "%s/logs", user->dir);
  file_mkdir(path);
      
  file_list_to_conn(conn, path);
}

void
read_user_log(User *user,
               struct mg_connection *conn, struct mg_request_info *req,
               char *fname) {
  char *path;
  path = user_log_path(user, fname);
  if (!path) {
    send_404(conn);
    return;
  }
      
  file_read_to_conn(conn, path, !strcasecmp(req->request_method,"HEAD"));
  free(path);
}
