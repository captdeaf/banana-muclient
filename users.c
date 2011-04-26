/* users.h
 *
 * User handling for banana.
 */

#include "banana.h"

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
user_guest(Session *session, char *username, char *userdir) {
  int i;
  int guestid;
  char pwfile[MAX_PATH_LEN];

  snprintf(pwfile, MAX_PATH_LEN, "%s/guestcount", userdir);

  pthread_mutex_lock(&user_mutex);

  // refcount < 0 means this is available for use.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount < 0) break;
  }

  if (i < MAX_USERS) {
    guestid = file_readnum(pwfile, 1);
    file_writenum(pwfile, guestid + 1);
    slog("Logging in guest %s-%d", username, guestid);
    // We do this inside of the mutex so we don't have any conflicts
    // with the user logging in simultaneously from two browser windows.
    memset(&users[i], 0, sizeof(User));
    snprintf(users[i].name, MAX_GUESTNAME_LEN, "%s-guest-%d",
             username, guestid);
    strncpy(users[i].dir, userdir, MAX_DIR_LEN);
    users[i].refcount = 1;
    if (pthread_mutex_init(&(users[i].mutex), &pthread_recursive_attr))
      perror("pthread_mutex_init");
    if (pthread_cond_init(&(users[i].evtAlarm), NULL))
      perror("pthread_cond_init");
    session->userid = i;
  }

  pthread_mutex_unlock(&user_mutex);

  if (i >= MAX_USERS) {
    return NULL;
  }

  return &users[i];
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
      if (!strcmp(users[i].name, username)
          && !strcmp(users[i].dir, userdir)) {
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
    strncpy(users[i].name, username, MAX_NAME_LEN);
    strncpy(users[i].dir, userdir, MAX_DIR_LEN);
    users[i].refcount = 1;
    if (pthread_mutex_init(&(users[i].mutex), &pthread_recursive_attr))
      perror("pthread_mutex_init");
    if (pthread_cond_init(&(users[i].evtAlarm), NULL))
      perror("pthread_cond_init");
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

  pthread_mutex_lock(&user_mutex);
  pthread_mutex_lock(&users[userid].mutex);
  users[userid].refcount--;
  if (users[userid].refcount == 0) {
    slog("User '%s' sessions expired.", users[userid].name);
  }
  pthread_mutex_unlock(&users[userid].mutex);
  pthread_mutex_unlock(&user_mutex);
}

static void
user_clean(User *user) {
  int j;
  pthread_mutex_lock(&user->mutex);
  if (user->refcount == 0) {
    slog("User '%s' cleaning up.", user->name);
  }

  for (j = 0; j < MAX_USER_EVENTS; j++) {
    if (user->events[j]) {
      event_free(user->events[j]);
    }
  }
  for (j = 0; j < MAX_USER_WORLDS; j++) {
    if (user->worlds[j].name[0]) {
      world_free(&user->worlds[j], 0);
    }
  }
 
  pthread_mutex_unlock(&user->mutex);
  pthread_cond_destroy(&user->evtAlarm);
  pthread_mutex_destroy(&user->mutex);
}

void
users_cleanup() {
  int i;
  pthread_mutex_lock(&user_mutex);
  // Go through all users and clean up those that have refcount == 0.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount == 0) {
      // Mark the user struct as ready to be reused.
      user_clean(&users[i]);
      users[i].refcount = -1;
    }
  }
  pthread_mutex_unlock(&user_mutex);
}

void
users_shutdown() {
  int i;
  pthread_mutex_lock(&user_mutex);
  // Go through all users and clean up those that have refcount == 0.
  for (i = 0; i < MAX_USERS; i++) {
    user_clean(&users[i]);
  }
  pthread_mutex_unlock(&user_mutex);
}

void
users_init() {
  int i;
  pthread_mutex_init(&user_mutex, &pthread_recursive_attr);
  memset(users, 0, sizeof(users));
  for (i = 0; i < MAX_USERS; i++) {
    users[i].refcount = -1;
  }
}
