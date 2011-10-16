/* users.h
 *
 * User handling for banana.
 */

#include "banana.h"

User users[MAX_USERS];  // Current users

// Protects users.
static pthread_mutex_t user_mutex;

// Get user object for the connection.
User *
user_get(Session *session) {
  User *ret = NULL;

  noisy_lock(&user_mutex, "users");

  if (session) {
    if (session->userid >= 0) {
      ret = &users[session->userid];
    }
  }

  noisy_unlock(&user_mutex, "users");

  return ret;
}

// Allocate new user object
User *
user_guest(Session *session, char *username, char *userdir) {
  int i;
  int guestid;
  char pwfile[MAX_PATH_LEN];

  snprintf(pwfile, MAX_PATH_LEN, "%s/guestcount", userdir);

  noisy_lock(&user_mutex, "users");

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
    snprintf(users[i].loginname, MAX_NAME_LEN, "%s", username);
    snprintf(users[i].name, MAX_GUESTNAME_LEN, "%s-guest-%d",
             username, guestid);
    strncpy(users[i].dir, userdir, MAX_DIR_LEN);
    users[i].timeout = 60;
    users[i].isGuest = 1;
    users[i].isAdmin = 0;
    users[i].refcount = 1;
    if (pthread_mutex_init(&(users[i].mutex), &pthread_recursive_attr))
      perror("pthread_mutex_init");
    if (pthread_cond_init(&(users[i].evtAlarm), NULL))
      perror("pthread_cond_init");
    session->userid = i;
  }

  noisy_unlock(&user_mutex, "users");

  if (i >= MAX_USERS) {
    return NULL;
  }

  return &users[i];
}

// Allocate new user object
User *
user_login(Session *session, char *username, char *userdir) {
  int i;
  char fpath[MAX_PATH_LEN];

  noisy_lock(&user_mutex, "users");

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
        noisy_unlock(&user_mutex, "users");
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
    snprintf(users[i].loginname, MAX_NAME_LEN, "%s", username);
    strncpy(users[i].name, username, MAX_NAME_LEN);
    strncpy(users[i].dir, userdir, MAX_DIR_LEN);
    snprintf(fpath, MAX_PATH_LEN, "%s/timeout", userdir);
    users[i].timeout = file_readnum(fpath, 120);
    users[i].isGuest = 0;
    // Is the user an admin?
    snprintf(fpath, MAX_PATH_LEN, "%s/is_admin", userdir);
    users[i].isAdmin = file_yorn(fpath);
    users[i].refcount = 1;
    if (pthread_mutex_init(&(users[i].mutex), &pthread_recursive_attr))
      perror("pthread_mutex_init");
    if (pthread_cond_init(&(users[i].evtAlarm), NULL))
      perror("pthread_cond_init");
    session->userid = i;
  }

  noisy_unlock(&user_mutex, "users");

  if (i >= MAX_USERS) {
    return NULL;
  }

  return &users[i];
}

void
user_expire(int userid) {
  if (userid < 0) return;

  noisy_lock(&user_mutex, "users");
  noisy_lock(&users[userid].mutex, users[userid].name);
  users[userid].refcount--;
  if (users[userid].refcount == 0) {
    slog("User '%s' sessions expired.", users[userid].name);
  }
  noisy_unlock(&users[userid].mutex, users[userid].name);
  noisy_unlock(&user_mutex, "users");
}

static void
user_clean(User *user) {
  int j;
  noisy_lock(&user->mutex, user->name);
  slog("User '%s' cleaning up.", user->name);

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
 
  noisy_unlock(&user->mutex, user->name);
  pthread_cond_broadcast(&(user->evtAlarm));
  pthread_cond_destroy(&(user->evtAlarm));
  pthread_mutex_destroy(&(user->mutex));
}

void
users_cleanup() {
  int i;
  noisy_lock(&user_mutex, "users");
  // Go through all users and clean up those that have refcount == 0.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount == 0) {
      // Mark the user struct as ready to be reused.
      user_clean(&users[i]);
      users[i].refcount = -1;
    }
  }
  noisy_unlock(&user_mutex, "users");
}

void
users_shutdown() {
  int i;
  noisy_lock(&user_mutex, "users");
  // Go through all users and clean up those that have refcount == 0.
  for (i = 0; i < MAX_USERS; i++) {
    if (users[i].refcount >= 0) {
      user_clean(&users[i]);
    }
  }
  noisy_unlock(&user_mutex, "users");
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
