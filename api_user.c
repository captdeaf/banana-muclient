/** api_user.c
 *
 * /action/user.foo api calls.
 */

#include "banana.h"

ACTION("user.setpassword", api_user_setpass, API_DEFAULT | API_NOLENGTH | API_NOGUEST) {
  char pwmd5[MD5_LEN];
  char *newpass;
  char pwfile[MAX_PATH_LEN];

  getvar(newpass, "newpassword", MAX_NAME_LEN);
  if (strlen(newpass) < 5) {
     sysMessage(user, "Password too short", newpass);
  }
  mg_md5(pwmd5, newpass, NULL);

  snprintf(pwfile, MAX_PATH_LEN, "%s/password.md5", user->dir);
  file_write(pwfile, pwmd5);
}

ACTION("user.gethost", api_user_gethost, API_AUTOHEADER) {
  char path[MAX_PATH_LEN];
  char *hostlimit;
  snprintf(path, MAX_PATH_LEN, "%s/host", user->dir);

  hostlimit = file_read(path);
  if (hostlimit) {
    mg_printf(conn, "%s", hostlimit);
    free(hostlimit);
  }
}

// We only get this if the user is already logged in. In which case:
// redirect them to their preferred client.
ACTION("login", api_logged_in, API_AUTOHEADER) {
  char clfile[MAX_PATH_LEN];
  char *client;
  snprintf(clfile, MAX_PATH_LEN, "%s/%s", user->dir, "client");
  client = file_read(clfile);
  mg_printf(conn,
       "<html><body>\n"
       "You are already logged in as '%s'.<br />\n"
       "<br />\n"
       "If you are connecting as a guest, please "
       "<a href=\"/action/logout\">Log Out</a> first.<br />\n"
       "<br />\n"
       "You can go back to your default client at:<br />\n"
       "<br />\n"
       "<a href=\"/%s\">%s</a>\n"
       "</body></html>",
         user->name,
         client ? client : "webfugue",
         client ? client : "webfugue"
       );
  if (client) {
    free(client);
  }
}

// Log out the user.
// redirect them to their preferred client.
ACTION("logout", api_logout, 0) {
  mg_printf(conn, "HTTP/1.1 302 Found\r\n"
            HEADER_NOCACHE
            "SetCookie: session=deleted; "
              "expires=Thu, 01-Jan-1970 00:00:01 GMT; path=/; httponly\r\n"
            "Location: /?loggedout=1\r\n\r\n");
}
