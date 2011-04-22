/* users.h
 *
 * User handling for banana.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <limits.h>

#include "mongoose.h"
#include "conf.h"
#include "banana.h"
#include "sessions.h"
#include "users.h"

static User users[MAX_USERS];  // Current users

// Protects users.
static pthread_mutex_t user_mutex;

// Get user object for the connection.
User *
user_get(const struct mg_connection *conn) {
  Session *session;
  User *ret = NULL;

  pthread_mutex_lock(&user_mutex);

  session = session_get(conn);
  if (session) {
    if (session->userid >= 0) {
      ret = &users[session->userid];
    }
  }

  pthread_mutex_unlock(&user_mutex);

  return ret;
}

// Allocate new user object
User *
user_login(Session *session, char *username, char *userdir) {
  int i;

  pthread_mutex_lock(&user_mutex);

  // First, see if the user is already logged in. If they are, point
  // the session at it and increase refcount.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount > 0) {
      if (!strcmp(users[i].username, username)
          && !strcmp(users[i].userdir, userdir)) {
        // We alrady have one. Bump up the refcount, unlock the mutex, and
        // return.
        users[i].refcount++;
        session->userid = i;
        pthread_mutex_unlock(&user_mutex);
        return &users[i];
      }
    }
  }

  // User is not already logged in. Create a new one, if we can.
  // refcount == -1 means this is available for use.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount < 0) break;
  }

  if (i < MAX_USERS) {
    // We do this inside of the mutex so we don't have any conflicts
    // with the user logging in simultaneously from two browser windows.
    memset(&users[i], 0, sizeof(User));
    strncpy(users[i].username, username, MAX_NAME_LEN);
    strncpy(users[i].userdir, userdir, MAX_DIR_LEN);
    users[i].refcount = 1;
    pthread_mutex_init(&(users[i].mutex), NULL);
    session->userid = i;
  }

  pthread_mutex_unlock(&user_mutex);

  if (i >= MAX_USERS) {
    return NULL;
  }

  return &users[i];
}

void
user_expire(int userid) {
  if (userid < 0) return;

  printf("Expiring user %d?\n", userid);
  pthread_mutex_lock(&user_mutex);
  pthread_mutex_lock(&users[userid].mutex);
  users[userid].refcount--;
  pthread_mutex_unlock(&users[userid].mutex);
  pthread_mutex_unlock(&user_mutex);
}

void
users_cleanup() {
  int i;
  pthread_mutex_lock(&user_mutex);
  // Go through all users and clean up those that have refcount == 0.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount == 0) {
      pthread_mutex_destroy(&users[i].mutex);
      printf("Cleaning user %d.\n", i);
      // TODO: Free events and worlds. Including removing sockets
      // from epoll.
     
      // Mark the user struct as ready to be reused.
      users[i].refcount = -1;
    }
  }
  pthread_mutex_unlock(&user_mutex);
}

void
users_init() {
  int i;
  pthread_mutex_init(&user_mutex, NULL);
  memset(users, 0, sizeof(users));
  for (i = 0; i < MAX_USERS; i++) {
    users[i].refcount = -1;
  }
}
