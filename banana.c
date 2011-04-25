/* banana.c
 *
 * The main glue for the Banana Web MU*ing API, using long polls/etc.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"
#include <signal.h>

static void
update_redirect(struct mg_connection *conn) {
  write_ajax_header(conn);
  mg_printf(conn, "window.location.replace('/index.html?err=Logged%%20out');\r\n");
}

ACTION("updates", handle_update, API_NOHEADER) {
  char *ucount;
  int updateCount;
  getvar(ucount, "updateCount", 20);

  updateCount = atoi(ucount);

  if (updateCount > user->updateCount) {
    // Shouldn't happen.
    update_redirect(conn);
    return;
  }

  if (updateCount < 0) {
    event_startup(user, conn);
  } else {
    event_wait(user, conn, updateCount);
  }
}

ACTION("login", handle_login, API_NOHEADER) {
  char *username;
  char *password;
  char pwfile[MAX_PATH_LEN];
  char pwmd5[MD5_LEN];
  char uri[MAX_PATH_LEN];
  char *pwcheck;
  int  ret;

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

  ret = !strncmp(pwcheck, password, 32);

  free(pwcheck);

  if (ret) {
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

  // User is logged in, created, etc etc. Send 'em on their way to their
  // client of choice. =).

  mg_printf(conn, "HTTP/1.1 302 FOUND\r\n");
  mg_printf(conn, "%s", HEADER_NOCACHE);
  mg_printf(conn, "%s\r\n", session->cookie_string);
  mg_printf(conn, "Location: %s\r\n\r\n", "/webcat");
}

static void *
event_handler(enum mg_event event, struct mg_connection *conn,
              struct mg_request_info *req) {
  char *retval = NULL;
  char *action;
  User *user;
  int i;
  if (event != MG_NEW_REQUEST) return NULL;

  // Todo: If we ever want ssl-only?
  // if (!req->is_ssl) {
  //   redirect_to_ssl(conn, req);
  // }
  init_conndata(conn, req);

  if (!strncmp(req->uri,"/action/", 8)) {
    action = req->uri + 8;

    // If this is a POST message, then we need to populate req->user_data.

    user = user_get(conn);
    if (user) {
      // Actions limited to users: Basicaly, all ACTION()s.
      // All logged-in actions are limited to POST.
      // if (strcasecmp(req->request_method,"POST"))
        // return NULL;

      // The most common action: Update.
      if (!strcmp(action, "updates")) {
        handle_update(user, conn, req, "updates");
        retval = "yes";
      }

      for (i = 0; allActions[i].name; i++) {
        if (!strcmp(allActions[i].name, action)) {
          allActions[i].handler(user, conn, req, allActions[i].name);

          // If the action doesn't have API_NOHEADER, then spew out
          // the ajax header.
          if (!(allActions[i].flags & API_NOHEADER)) {
            write_ajax_header(conn);
          }
          retval = "yes";
          break;
        }
      }
    } else {
      // Actions limited to non-users.
      if (!strcmp(action, "login")) {
        handle_login(NULL, conn, req, "login");
        retval = "yes";
      } else if (!strcmp(action, "guest")) {
        // handle_guest(conn, req);
        // retval = "yes";
      } else if (!strcmp(action, "updates")) {
        update_redirect(conn);
        retval = "yes";
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
