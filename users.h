/** users.h
 *
 * Defines users for Banana, including their worlds.
 */

#ifndef _MY_USER_H_
#define _MY_USER_H_

typedef struct user {
  // Username: guest-1, walker, etc.
  char name[MAX_NAME_LEN];
  // User directory: users/foo/bar
  char dir[MAX_DIR_LEN];
  // A refcount == -1 means this user struct is available for reassignment.
  // A refcount == 0 means this user struct needs to be freed.
  // A refcount > 0 means it's in use.
  int refcount;

  // User mutex. For protecting worlds and events.
  pthread_mutex_t mutex;
  pthread_cond_t  evtAlarm;

  // Mu* connections.
  struct world worlds[MAX_USER_WORLDS];

  // This is a ringbuffer, so we have evtS and evtE. evtS points at the
  // first / earliest one. evtE points at where the next will go.
  // At start: evtE == evtS. We free events[evtS] and bump evtS when
  // (evtE+1)%MAX_USER_EVENTS == evtS.
  // After the buffer fills up, evtS will == (evtE+1)%MAX_USER_EVENTS.
  struct event *events[MAX_USER_EVENTS];
  int evtS, evtE;

  // updateCount: Total # of events so far for this user. The <updateCount>th
  // event is always evtE.
  int updateCount;
} User;

User *user_get(const struct mg_connection *conn);
User *user_login(Session *session, char *username, char *userdir);

// Called after session_expire, it is given a userid to decrease th
// refcount of.
void user_expire(int userid);

// Initialize user stuff.
void users_init();
// Walk through the users struct and clean up any that are marked for
// deletion. (refcount == 0)
void users_cleanup();

#endif
