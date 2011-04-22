/* worlds.c
 *
 * World handling for banana.
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
#include "conf.h"
#include "util.h"
#include "banana.h"
#include "sessions.h"
#include "worlds.h"
#include "users.h"
#include "events.h"

/*
typedef struct world {
  // World name.
  char worldname[MAX_NAME_LEN];

  struct event *events[MAX_WORLD_EVENTS];
  int evtS, evtE;
} World;
*/

void
world_open(User *user, char *worldname) {
  int i;
  World *world;
  if (!valid_name(worldname)) {
    sysMessage(user, "Invalid world name '%s'.", worldname);
    return;
  }
  pthread_mutex_lock(&user->mutex);
  for (i = 0; i < MAX_USER_WORLDS; i++) {
    if (user->worlds[i].name == worldname) {
      sysMessage(user, "World '%s' already exists.", worldname);
    }
  }
  if (i >= MAX_USER_WORLDS) {
    for (i = 0; i < MAX_USER_WORLDS; i++) {
      if (user->worlds[i].name[0] == 0) {
        world = &user->worlds[i];
        // Allocate this world.
        memset(world, 0, sizeof(World));
        snprintf(world->name, MAX_NAME_LEN, "%s", worldname);
        world->openTime = time(NULL);
        addEvent(user, NULL, EVENT_WORLD_OPEN,
            "world: '%s', status: 'disconnected', lines: 0, "
            "openTime: %d", world->name, world->openTime);
        break;
      }
    }
    if (i >= MAX_USER_WORLDS) {
      sysMessage(user, "Too many worlds.");
    }
  }
  pthread_mutex_unlock(&user->mutex);
}
