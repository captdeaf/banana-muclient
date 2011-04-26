/** api_user.c
 *
 * /action/user.foo api calls.
 */

#include "banana.h"

ACTION("user.setpassword", api_user_setpass, API_DEFAULT) {
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
