/** worlds.h
 *
 * Defines worlds for Banana, including their worlds.
 */

#ifndef _MY_WORLD_H_
#define _MY_WORLD_H_

typedef struct world {
  // World name.
  char name[MAX_NAME_LEN];

  unsigned int openTime;
  unsigned int connectTime;

  // Ditto as user->events/evtS/evtE
  struct event *events[MAX_WORLD_EVENTS];
  int evtS, evtE;
} World;

struct user;
void world_open(struct user *user, char *worldname);
#endif
