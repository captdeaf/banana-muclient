/* conf.h
 *
 * Just so we can keep our configuration options in conf.c for easy editing
 * before compiling.
 */

#ifndef _MY_CONF_H_
#define _MY_CONF_H_

extern const char *options[];

extern int   use_ssl;
extern char *login_url;
extern char *session_salt;
// extern char *ssl_url;

// Web sessions, that expire.
#define MAX_SESSIONS 30

// How long until a session expires?
#define SESSION_TTL 120

// Seconds between checks for expiring sessions/users.
#define CLEANUP_TIMEOUT 5

// Active users at a time, not all users on disk.
#define MAX_USERS    10

// A hard max of world count for any one user.
#define MAX_USER_WORLDS 2

// Max events kept on a per-world basis. Worlds _only_
// keep their receive and 
#define MAX_WORLD_EVENTS 300

// Max events kept by a user.
#define MAX_USER_EVENTS 1000

// File i/o. 1MB maximum filesize. Real MB, not 1000000.
#define MAX_FILE_SIZE (1024*1024)

// String lengths:
// walker
#define MAX_NAME_LEN 30
// users/walker
#define MAX_DIR_LEN (MAX_NAME_LEN + 10)
// users/walker/files/filename
#define MAX_PATH_LEN (MAX_NAME_LEN * 2 + 20)

#define MD5_LEN 33

#endif
