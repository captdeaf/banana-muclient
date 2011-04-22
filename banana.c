/* banana.c
 *
 * The main glue for the Banana Web MU*ing API, using long polls/etc.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "mongoose.h"
#include "conf.h"
#include "sessions.h"
#include "worlds.h"
#include "users.h"
#include "banana.h"
#include "util.h"
#include "api.h"
#include "file.h"
#include "events.h"
#include "api_inc.h"

static void
update_redirect(struct mg_connection *conn) {
  write_ajax_header(conn);
  mg_printf(conn, "window.location.replace('/index.html?err=Logged%%20out');\r\n");
}

static void
handle_update(struct user *user, struct mg_connection *conn,
              const struct mg_request_info *req) {
  char ucount[20];
  int updateCount;
  if (!get_qsvar(req, "updateCount", ucount, 20)) {
    update_redirect(conn);
    return;
  }
  updateCount = atoi(ucount);
  if (updateCount > user->updateCount) {
    // Shouldn't happen.
    update_redirect(conn);
    return;
  }
  if (updateCount < 0) {
    // TODO: Dump open and world events.
    mg_printf(conn, update_header, 0);
  } else {
    event_wait(user, conn, updateCount);
  }
}

static void
handle_login(struct mg_connection *conn,
             const struct mg_request_info *req) {
  char username[MAX_NAME_LEN];
  char password[MAX_NAME_LEN];
  char pwfile[MAX_PATH_LEN];
  char pwmd5[MD5_LEN];
  char uri[MAX_PATH_LEN];
  char *pwcheck;
  int  ret;

  Session *session;
  User *user;

  get_qsvar(req, "username", username, MAX_NAME_LEN);
  get_qsvar(req, "password", password, MAX_NAME_LEN);

  // Make sure both username and password are supplied.
  if (!username[0]) { redirect_to(conn, "/index.html"); return; }
  if (!password[0]) { redirect_to(conn, "/index.html?err=No%20password%20provided"); return; }

  printf("User '%s' Attempting to log in.\n", username);
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
    snprintf(uri, MAX_PATH_LEN, "/index.html?user=%s&err=Invalid%%20password", username);
    redirect_to(conn, uri);
    return;
  }

  session = session_make();
  if (!session) { redirect_to(conn, "/nosessions.html"); return; }

  snprintf(pwfile, MAX_PATH_LEN, "users/%s", username);
  user = user_login(session, username, pwfile);
  if (!user) { redirect_to(conn, "/nousers.html"); return; }

  // User is logged in, created, etc etc. Send 'em on their way to their
  // client of choice. =).

  mg_printf(conn, "HTTP/1.1 302 FOUND\r\n");
  mg_printf(conn, "Cache: no-cache\r\n");
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
  req->user_data = NULL;

  if (!strncmp(req->uri,"/action/", 8)) {
    action = req->uri + 8;

    // If this is a POST message, then we need to populate req->user_data.
    if (!strcasecmp(req->request_method,"POST")) {
      req->user_data = read_post_data(conn);
    }

    user = user_get(conn);
    if (user) {
      // Actions limited to users: Basicaly, all ACTION()s.
      // All logged-in actions are limited to POST.
      // if (strcasecmp(req->request_method,"POST"))
        // return NULL;

      // The most common action: Update.
      if (!strcmp(action, "updates")) {
        handle_update(user, conn, req);
        retval = "yes";
      }

      for (i = 0; allActions[i].name; i++) {
        if (!strcmp(allActions[i].name, action)) {
          allActions[i].handler(user, conn, req);
          retval = "yes";
          break;
        }
      }
    } else {
      printf("We have no user and we must cry. :-(!\n");
      // Actions limited to non-users.
      if (!strcmp(action, "login")) {
        handle_login(conn, req);
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
  if (req->user_data) {
    // Free the read post data.
  }
  return retval;
}

int is_running = 1;
int musockets_fd = -1;

#define MAX_EVENTS 20
#define WAIT_TIMEOUT 1000
void
poll_musockets() {
  struct epoll_event events[MAX_EVENTS];
  time_t now = time(NULL);
  time_t next = time(NULL) + CLEANUP_TIMEOUT;
  
  while (is_running) {
    epoll_wait(musockets_fd, events, MAX_EVENTS, WAIT_TIMEOUT);
    // TODO: Handle events. Whee.
    now = time(NULL);
    if (now >= next) {
      sessions_expire();
      users_cleanup();
      next = now + CLEANUP_TIMEOUT;
    }
  }
}

void
session_user_expire(Session *session) {
  if (session->userid >= 0)
    user_expire(session->userid);
}

int
main(int argc _unused_, char *argv[] _unused_) {
  struct mg_context *ctx;

  // Initialize random number generator. It will be used later on for
  // the session identifier creation.
  srand((unsigned) time(0));

  // Util needs to be initialized before everything else, because it seta
  // pthread_attr_recursive
  util_init();

  // Initialize sessions.
  sessions_init(session_user_expire);
  users_init();

  // Initialize musockets_fd.
  musockets_fd = epoll_create(50);
  if (musockets_fd < 0) {
    perror("epoll_create");
    return 1;
  }

  // Setup and start Mongoose
  ctx = mg_start(&event_handler, NULL, options);
  assert(ctx != NULL);

  // Wait until enter is pressed, then exit
  printf("Banana server started on ports %s, press enter to quit.\n",
         mg_get_option(ctx, "listening_ports"));

  // Begin the loop sitting on epoll
  poll_musockets();

  mg_stop(ctx);
  return EXIT_SUCCESS;
}
