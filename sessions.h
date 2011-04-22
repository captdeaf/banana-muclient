/** mg_session.h
 *
 * Contains all the information we need for a single session.
 */

#ifndef _MY_SESSION_H_
#define _MY_SESSION_H_

typedef struct _session {
  // Identifying the web session
  char session_id[33];      // Session ID, must be unique
  char random[20];          // Random data used for extra user validation
  char cookie_string[100];   // The cookie string.

  // Time left.
  time_t expire;            // Expiration timestamp, UTC. expire of 0
                            // Means that this session is unused.

  // The user ID.
  int    userid;            // -1 until authorized.
} Session;

// Fetch a current session for a connection.
Session *session_get(const struct mg_connection *conn);

// Create one: Only call if it doesn't have one.
// It returns a set cookie_string. Don't forget to SetCookie it!
Session *session_make();

// A callback for when a session expires.
typedef void (*on_session_free)(Session *);

// Called on startup.
void sessions_init(on_session_free cleaner);

// Call this every once in a while to expire old sessions. If a session
// expires, it will call cleaner(<session>) before doing its own cleanup.
void sessions_expire();

#endif
