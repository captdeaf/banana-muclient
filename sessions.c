/* session.c
 *
 * Session handling for banana.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include "banana.h"

static Session sessions[MAX_SESSIONS];  // Current sessions

// Protects sessions.
static pthread_mutex_t session_mutex;

// Get session object for the connection.
Session *
session_get(const struct mg_connection *conn) {
  int i;
  char session_id[SESSION_LEN];
  time_t now = time(NULL);
  noisy_lock(&session_mutex, "sessions");

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

  noisy_unlock(&session_mutex, "sessions");
  return i == MAX_SESSIONS ? NULL : &sessions[i];
}

// Allocate new session object
static Session *
session_new(void) {
  int i;
  time_t now = time(NULL);
  noisy_lock(&session_mutex, "sessions");
  for (i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].expire == 0 || sessions[i].expire < now) {
      sessions[i].expire = time(NULL) + SESSION_TTL;
      break;
    }
  }
  noisy_unlock(&session_mutex, "sessions");
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
    snprintf(session->random, RANDOM_LEN, "%d", rand());
    generate_session_id(session->session_id, session->random, session_salt);
    snprintf(session->cookie_string, COOKIE_LEN,
             "Set-Cookie: session=%s; path=/; Expires=Wed, 06-Jan-2038 12:34:56 GMT",
             session->session_id);
    slog("Session: '%s' created", session->session_id);
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
  noisy_lock(&session_mutex, "sessions");
  for (i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].expire > 0 && sessions[i].expire < now) {
      slog("Session: '%s' expired", sessions[i].session_id);
      if (session_cleaner) {
        session_cleaner(&sessions[i]);
      }
      sessions[i].expire = 0;
      sessions[i].userid = -1;
    }
  }
  noisy_unlock(&session_mutex, "sessions");
}
