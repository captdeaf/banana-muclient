/** mg_session.h
 *
 * Contains all the information we need for a single session.
 */

#ifndef _MY_SESSION_H_
#define _MY_SESSION_H_

#define SESSION_LEN 33
#define RANDOM_LEN  20
#define COOKIE_LEN  200
typedef struct session {
  // Identifying the web session
  char session_id[SESSION_LEN];      // Session ID, must be unique
  char random[RANDOM_LEN];          // Random data used for extra user validation
  char cookie_string[COOKIE_LEN];   // The cookie string.

  // Time left.
  time_t expire;            // Expiration timestamp, UTC. expire of 0
                            // Means that this session is unused.

  // The user ID.
  int    userid;            // -1 until authorized.
} Session;

// Fetch a current session for a connection.
Session *session_get(const struct mg_connection *conn);

void session_logout(Session *);

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
