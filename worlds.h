/** worlds.h
 *
 * Defines worlds for Banana, including their worlds.
 */

#ifndef _MY_WORLD_H_
#define _MY_WORLD_H_

#define ANSI_UNDERLINE  0x01 /* <u> */
#define ANSI_FLASH      0x02 /* <em> */
#define ANSI_HILITE     0x04 /* <strong> */
#define ANSI_INVERT     0x08 /* Switch bg and fg */

#define ANSI_RGB        0x80 /* not supported yet: Uses style="<color>" instead
                          * of span classes */
#define ANSI_SIZE 12
struct a2h {
  char f[ANSI_SIZE]; /* fg_r, fg_25. */
  char b[ANSI_SIZE]; /* bg_r */
  int flags;
};

typedef struct world {
  struct user *user;
  // World name.
  char name[MAX_NAME_LEN];
  int tried_iac;
  char charset[20];

  // Logger for the world.
  Logger *logger;

  // Ditto as user->events/evtS/evtE
  struct event *events[MAX_WORLD_EVENTS];
  int evtS, evtE;
  // Number of receive, prompt and echo events the world has received.
  unsigned int lineCount;

  // Network stuff.
  // World can be closed at any time: Disconnecting the socket.
  // World open: invalid.
  // World connect: Only when disconnected. It sets status to connecting.
  // World send: Only when connected, it keeps the lock until send succeeds.
  // World echo: Independent of netstatus.
  // World disconnect: On any status but disconnect.
  // World close: On any status.
  // User cleanup == world close.
#define WORLD_DISCONNECTED 0
#define WORLD_CONNECTING   1
#define WORLD_CONNECTED    2
  int   netstatus;
  // The socketnum is a way to tell the connecting thread that status has
  // changed. It checks socketnum to ensure it doesn't try to work with an
  // invalid connection.
  pthread_t connthread;
  pthread_mutex_t sockmutex;
  char  ipaddr[49];
  int   fd;
  char *host; // Remote host.
  int   port;
  char  buff[BUFFER_LEN];
  int   lbp;
  struct a2h   a2h;
  unsigned int openTime;
  unsigned int connectTime;
  // Number of incoming lines from the socket.
  unsigned int netlineCount;
} World;

struct user;
char *world_status(struct world *world);
void world_open(struct user *user, char *worldname);
void world_dc_event(struct user *user, struct world *w);
void world_free(struct world *world, int doevents);
void world_close(struct user *user, char *worldname);
void world_send(struct user *user, char *worldname, char *text);
void world_echo(struct user *user, char *worldname, char *text);
void world_connect(struct user *user, char *worldname, char *host, char *port);
void world_disconnect(struct user *user, char *worldname);
#endif
