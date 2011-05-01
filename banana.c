/* banana.c
 *
 * The main glue for the Banana Web MU*ing API, using long polls/etc.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"
#include "genapi.h"
#include <signal.h>

// Redirect the user to their preferred client.
ACTION("updates", handle_update, API_NONE) {
  char *ucount;
  int updateCount;
  getvar(ucount, "updateCount", 20);

  updateCount = atoi(ucount);

  if (updateCount > user->updateCount) {
    // Shouldn't happen, but may be caused by a browser accessing
    // an old cache.
    // update_redirect(conn);
    send_error(conn, "Turn off caches.");
    return;
  }

  if (updateCount < 0) {
    event_startup(user, conn);
  } else {
    event_wait(user, conn, updateCount);
  }
}

void
handle_guest(struct mg_connection *conn, struct mg_request_info *req _unused_,
             char *uname) {
  char uri[MAX_PATH_LEN];
  char pwfile[MAX_PATH_LEN];
  char username[MAX_NAME_LEN];
  User *user;

  Session *session;
  user = NULL;

  // Create our 
  snprintf(username, MAX_NAME_LEN, "%s", uname);

  if (username == NULL) {
    redirect_to(conn, "/index.html");
    return;
  }
  if (username == VAR_TOO_LONG) {
    redirect_to(conn, "/index.html?err=Username%20too%20long");
    return;
  }

  // valid_name will turn username into a lowercase representation.
  if (!valid_name(username)) {
    redirect_to(conn, "/index.html?err=Invalid%20username");
    return;
  }

  // Make sure the user exists, by checking for password.md5 in their
  // directory.
  snprintf(pwfile, MAX_PATH_LEN, "users/%s/password.md5", username);
  if (!file_exists(pwfile)) {
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=Unknown%%20User", username);
    redirect_to(conn, uri);
    return;
  }

  // Make sure the account is allowed for guesting.
  snprintf(pwfile, MAX_PATH_LEN, "users/%s/can_guest", username);
  if (!file_yorn(pwfile)) {
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=User%%20cannot%%20guest", username);
    redirect_to(conn, uri);
    return;
  }

  // At this point, we have a valid guest. Give them a guest-y name.
  session = session_make();
  if (!session) {
    slog("Guest user '%s': Out of sessions.", username);
    redirect_to(conn, "/nosessions.html");
    return;
  }

  snprintf(pwfile, MAX_PATH_LEN, "users/%s", username);
  user = user_guest(session, username, pwfile);
  if (!user) {
    slog("Guest user '%s': Out of users.", username);
    redirect_to(conn, "/nousers.html");
    return;
  }
  slog("Session: Guest user '%s' attached to session '%s'",
       user->name, session->session_id);

  // User is logged in, created, etc etc. Send 'em on their way to their
  // client of choice. =).
  redirect_to_client(session, user, conn);
}

void
handle_login(struct mg_connection *conn, struct mg_request_info *req) {
  char *username;
  char *password;
  char pwfile[MAX_PATH_LEN];
  char pwmd5[MD5_LEN];
  char uri[MAX_PATH_LEN];
  char *pwcheck;
  int  ret;

  User *user;
  Session *session;
  user = NULL;

  username = get_qsvar(req, "username", MAX_NAME_LEN);
  password = get_qsvar(req, "password", MAX_NAME_LEN);

  if (username == NULL) {
    redirect_to(conn, "/index.html");
    return;
  }
  if (username == VAR_TOO_LONG) {
    redirect_to(conn, "/index.html?err=Username%20too%20long");
    return;
  }
  if (password == NULL) {
    redirect_to(conn, "/index.html?err=No%20password%20provided");
    return;
  }
  if (password == VAR_TOO_LONG) {
    redirect_to(conn, "/index.html?err=Password%20too%20long");
    return;
  }

  // Make sure both username and password are supplied.

  // valid_name will turn username into a lowercase representation.
  if (!valid_name(username)) {
    redirect_to(conn, "/index.html?err=Invalid%20username");
    return;
  }

  // Make sure the user exists, by checking for password.md5 in their
  // directory.
  snprintf(pwfile, MAX_PATH_LEN, "users/%s/password.md5", username);
  if (!file_exists(pwfile)) {
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=Unknown%%20User", username);
    redirect_to(conn, uri);
    return;
  }

  // At this point, we have a valid user and their password.
  // Encrypt and check.
  mg_md5(pwmd5, password, NULL);
  pwcheck = file_read(pwfile);
  if (!pwcheck) {
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=Unknown%%20error", username);
    redirect_to(conn, uri);
    return;
  }

  ret = !strncmp(pwcheck, pwmd5, 32);

  free(pwcheck);

  if (!ret) {
    slog("User '%s' failed password check.", username);
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=Invalid%%20password", username);
    redirect_to(conn, uri);
    return;
  }

  session = session_make();
  if (!session) { redirect_to(conn, "/nosessions.html"); return; }

  snprintf(pwfile, MAX_PATH_LEN, "users/%s", username);
  user = user_login(session, username, pwfile);
  if (!user) {
    slog("User '%s': Out of users.", username);
    redirect_to(conn, "/nousers.html"); return;
  }
  slog("Session: User '%s' attached to session '%s'",
       username, session->session_id);

  redirect_to_client(session, user, conn);
}

static void *
event_handler(enum mg_event event, struct mg_connection *conn,
              struct mg_request_info *req) {
  char *retval = NULL;
  char *action;
  User *user = NULL;
  Session *session = NULL;
  int i;
  if (event != MG_NEW_REQUEST) return NULL;

  // Todo: If we ever want ssl-only?
  // if (!req->is_ssl) {
  //   redirect_to_ssl(conn, req);
  // }
  init_conndata(conn, req);

  session = session_get(conn);
  if (session) {
    user = user_get(session);
  }

  if (!strncmp(req->uri,"/action/", 8)) {
    action = req->uri + 8;

    // If this is a POST message, then we need to populate req->user_data.

    if (user) {
      // Actions limited to users: Basicaly, all ACTION()s.
      // All logged-in actions are limited to POST.
      // if (strcasecmp(req->request_method,"POST"))
        // return NULL;

      // The most common action: Update.
      if (!strcmp(action, "updates")) {
        handle_update(user, conn, req, "updates");
        retval = "yes";
      } else if (!strcmp(action, "logout")) {
        slog("Session: User '%s' logged out of session '%s'.",
             user->name, session->session_id);
        api_logout(user, conn, req, "logout");
        session_logout(session);
        retval = "yes";
      } else {
        for (i = 0; allActions[i].name; i++) {
          if (!strcmp(allActions[i].name, action)) {
            allActions[i].handler(user, conn, req, allActions[i].name);

            // If the action has API_AUTOHEADER set, then spit out
            // the ajax header.
            if (allActions[i].flags & API_AUTOHEADER) {
              write_ajax_header(conn, allActions[i].flags);
            }
            retval = "yes";
            break;
          }
        }
      }
    } else {
      // Actions limited to non-users.
      if (!strcmp(action, "login")) {
        handle_login(conn, req);
        retval = "yes";
      } else if (!strcmp(action, "updates")) {
        send_error(conn, "You are logged out.");
        retval = "yes";
      }
    }
  } else if (!strncmp(req->uri,"/guest/", 7)) {
    if (user) {
      redirect_to_client(NULL, user, conn);
    } else {
      action = req->uri + 7;
      if (*action) {
        handle_guest(conn, req, action);
        retval = "yes";
      }
    }
  } else if (!strncmp(req->uri, "/user/", 6)) {
    if (user) {
      action = req->uri + 6;
      // Try for /user/<username>/... And we only accept when <username>
      // == the logged in user.
      int len = strlen(user->loginname);
      if (!strncmp(action, user->loginname, len) && (*(action+len) == '/')) {
        action = action + len + 1; // Advance past the /
        if (!strncmp(action, "files/", 6)) {
          action = action + 6;
          if (action[0]) {
            action += 1;
            retval = "yes";
            read_user_file(user, conn, req, action);
          } else {
            retval = "yes";
            list_user_files(user, conn, req);
          }
        } else if (!strncmp(action, "logs/", 5)) {
          action = action + 5;
          if (action[0]) {
            action += 1;
            retval = "yes";
            read_user_log(user, conn, req, action);
          } else {
            retval = "yes";
            list_user_logs(user, conn, req);
          }
        }
      }
    }
  }
  free_conndata(conn, req);
  return retval;
}

void
session_user_expire(Session *session) {
  if (session->userid >= 0) {
    user_expire(session->userid);
  }
}

struct mg_context *ctx;

void
do_shutdown(int signal _unused_) {
  // kill the webserver. Or not, it causes shutdown to hang,
  // and we don't really need it to do anything to clean up, anyway.
  // mg_stop(ctx);

  slog("Cleaning up users.");
  // Clean up all users.
  users_shutdown();

  slog("Goodbye.");
  // Finally, shut down logs.
  logger_shutdown();
  exit(1);
}

int
main(int argc _unused_, char *argv[] _unused_) {
  // Initialize random number generator. It will be used later on for
  // the session identifier creation.
  srand((unsigned) time(0));

  // Setup and start Mongoose
  ctx = mg_start(&event_handler, NULL, options);
  assert(ctx != NULL);

  logger_init(LOG_PATH);
  slog("Banana started.");
  // Util needs to be initialized before everything else, because it seta
  // pthread_attr_recursive
  util_init();

  // Initialize sessions.
  sessions_init(session_user_expire);
  users_init();

  signal(SIGTERM, do_shutdown);
  signal(SIGINT, do_shutdown);
  signal(SIGHUP, do_shutdown);
  signal(SIGQUIT, do_shutdown);

  // Initialize the network.
  if (net_init()) {
    return 1;
  }

  // Wait until enter is pressed, then exit
  slog("Banana server started on ports %s. ^C to quit.\n",
         mg_get_option(ctx, "listening_ports"));

  // Begin the loop sitting on epoll
  net_poll();

  return EXIT_SUCCESS;
}
