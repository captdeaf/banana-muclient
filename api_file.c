/** api_file.c
 *
 * /action/file.foo api calls.
 */

#include "banana.h"

static char *
user_file_path(User *user, char *fname) {
  char path[100];
  if (!valid_name(fname)) return NULL;
  snprintf(path, 100, "%s/files", user->dir);
  file_mkdir(path);
  snprintf(path, 100, "%s/files/%s", user->dir, fname);
  return strdup(path);
}

ACTION("file.write", api_file_write, API_NOGUEST) {
  char *file;
  char *contents;
  char *path;
  getvar(file, "file", MAX_NAME_LEN);
  getvar(contents, "contents", MAX_FILE_SIZE);
  path = user_file_path(user, file);
  char *jfile = json_escape(file);
  if (!path) {
    addEvent(user, NULL, EVENT_FILE_WRITE_FAIL,
             "filename:'%s',cause:'Invalid file name.'",
             jfile);
  } else {
    file_write(path, contents);
    addEvent(user, NULL, EVENT_FILE_WRITE, "filename:'%s'", jfile);
    free(path);
  }
  free(jfile);
}

void
list_user_files(User *user,
                struct mg_connection *conn,
                struct mg_request_info *req _unused_) {
  char path[100];
  snprintf(path, 100, "%s/files", user->dir);
  file_mkdir(path);
      
  file_list_to_conn(conn, path);
}

void
read_user_file(User *user,
               struct mg_connection *conn, struct mg_request_info *req,
               char *fname) {
  char *path;
  path = user_file_path(user, fname);
  if (!path) {
    send_404(conn);
    return;
  }
      
  file_read_to_conn(conn, path, !strcasecmp(req->request_method,"HEAD"));
  free(path);
}
