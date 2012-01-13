/* conf.h
 *
 * Just so we can keep our configuration options in conf.c for easy editing
 * before compiling.
 */

#ifndef _MY_CONF_H_
#define _MY_CONF_H_

#define LOG_PATH "logs/banana"

/////////////////////////////////////////////////
// HTTP Configuration
/////////////////////////////////////////////////

// Which ports do we listen on for HTTP? This is a string since it
// is passed to mongoose. Use "1234s" to listen on HTTPS on port
// 1234. If you have an ssl port, then USE_SSL must be defined,
// and SSL_PEM_FILE must contain the name of a valid .pem file.

#define HTTP_PORTS "8088,9099s"

// How many seconds do we wait on a socket before returning?
#define LONG_POLL_SECONDS 30

#define USE_SSL
#define SSL_PEM_FILE "ssl_cert.pem"

// The directory containing the static files.
#define DOCUMENT_ROOT "html"
// Base URL path for client HTTP requests.
#define SEND_BASE_URL_PATH "/"
// Base URL path the server expects.
#define RECEIVE_BASE_URL_PATH "/"

/////////////////////////////////////////////////
// Network Configuration
/////////////////////////////////////////////////

// Seconds between checks for expiring sessions/users.
#define CLEANUP_TIMEOUT     5

// Don't delete this, it's just for declaring what's in conf.c
extern const char *options[];

/////////////////////////////////////////////////
// User and Session limits.
/////////////////////////////////////////////////

// How many seconds until a session expires?
#define SESSION_TTL       120

// A string to salt the md5 encryption of a random string with.
#define SESSION_SALT    "bananawhams"

// Web sessions, or active logins.
#define MAX_SESSIONS       40

// NUM_THREADS should be MAX_SESSIONS + <a good number>
// It is also a string, since that's what Mongoose expects.
#define NUM_THREADS       "50"

// Active users at a time, not all users on disk.
#define MAX_USERS          30

// A hard max of world count for any one user.
#define MAX_USER_WORLDS    10

// Max events kept on a per-world basis. Worlds _only_
// keep their receive and prompt events.
#define MAX_WORLD_EVENTS 1000

// Max events kept by a user object. This is used only to keep current
// connections up to date. If you are seeing too many Overflow events,
// then up this.
#define MAX_USER_EVENTS 5000

/////////////////////////////////////////////////
// Buffer and File limits.
/////////////////////////////////////////////////

// File I/O. 1MB maximum filesize. Real MB, not 1000000.
#define MAX_FILE_SIZE (1024*1024)

// Maximum size a buffer can have. Any lines over this get
// truncated.
#define READ_LEN (8192*2)
#define BUFFER_LEN (8192*2)

// Maximum size for a single event.
#define MAX_EVENT_SIZE (BUFFER_LEN*2)

// walker
#define MAX_NAME_LEN 30
// guestname len is max_name_len + "guest-<num>"
#define MAX_GUESTNAME_LEN 50
// users/walker
#define MAX_DIR_LEN (MAX_NAME_LEN + 10)
// users/walker/files/filename
#define MAX_PATH_LEN (MAX_NAME_LEN * 2 + 20)

// Number of characters in an md5 string, including NULL char.
#define MD5_LEN 33

#endif
