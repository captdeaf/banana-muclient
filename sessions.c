/* mg_session.c
 *
 * Session handling for banana.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
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
#include "util.h"
#include "banana.h"
#include "sessions.h"
#include "conf.h"

static Session sessions[MAX_SESSIONS];  // Current sessions

// Protects sessions.
static pthread_mutex_t session_mutex;

// Get session object for the connection.
Session *
session_get(const struct mg_connection *conn) {
  int i;
  char session_id[33];
  time_t now = time(NULL);
  pthread_mutex_lock(&session_mutex);

  mg_get_cookie(conn, "session", session_id, sizeof(session_id));
  for (i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].expire != 0 &&
        sessions[i].expire > now &&
        strcmp(sessions[i].session_id, session_id) == 0) {
      // Bump sessions expire time, since this was accessed.
      sessions[i].expire = time(NULL) + SESSION_TTL;
      break;
    }
  }

  pthread_mutex_unlock(&session_mutex);
  return i == MAX_SESSIONS ? NULL : &sessions[i];
}

// Allocate new session object
static Session *
session_new(void) {
  int i;
  time_t now = time(NULL);
  pthread_mutex_lock(&session_mutex);
  for (i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].expire == 0 || sessions[i].expire < now) {
      sessions[i].expire = time(NULL) + SESSION_TTL;
      break;
    }
  }
  pthread_mutex_unlock(&session_mutex);
  if (i >= MAX_SESSIONS) {
    return NULL;
  }
  sessions[i].expire = time(NULL) + SESSION_TTL;
  sessions[i].userid = -1;
  return &sessions[i];
}

// Generate session ID. buf must be 33 bytes in size.
// Note that it is easy to steal session cookies by sniffing traffic.
// This is why all communication must be SSL-ed.
static void
generate_session_id(char *buf, const char *random, const char *salt) {
  mg_md5(buf, random, salt, NULL);
}

// A handler for the /authorize endpoint.
// Login page form sends user name and password to this endpoint.
Session *
session_make() {
  Session *session;

  session = session_new();
  if (session) {
    snprintf(session->random, sizeof(session->random), "%d", rand());
    generate_session_id(session->session_id, session->random, session_salt);
    snprintf(session->cookie_string, 100,
             "Set-Cookie: session=%s",
             session->session_id);
  }
  return session;
}

on_session_free session_cleaner = NULL;

void
sessions_init(on_session_free cleaner) {
  memset(sessions, 0, sizeof(sessions));
  session_cleaner = cleaner;
  pthread_mutex_init(&session_mutex, &pthread_recursive_attr);
}

void
sessions_expire() {
  int i;
  time_t now = time(NULL);
  for (i = 0; i < MAX_SESSIONS; i++) {
    pthread_mutex_lock(&session_mutex);
    if (sessions[i].expire > 0 && sessions[i].expire < now) {
      if (session_cleaner) {
        session_cleaner(&sessions[i]);
      }
      sessions[i].expire = 0;
      sessions[i].userid = -1;
    }
    pthread_mutex_unlock(&session_mutex);
  }
}
