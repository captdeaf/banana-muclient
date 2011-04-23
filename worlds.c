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
#include "net.h"

/*
typedef struct world {
  // World name.
  char worldname[MAX_NAME_LEN];

  struct event *events[MAX_WORLD_EVENTS];
  int evtS, evtE;
} World;
*/

World *
world_get(User *user, char *worldname) {
  int i;
  if (!valid_name(worldname)) {
    sysMessage(user, "Invalid world name '%s'.", worldname);
    return NULL;
  }
  for (i = 0; i < MAX_USER_WORLDS; i++) {
    if (!strcmp(user->worlds[i].name, worldname)) {
      return &(user->worlds[i]);
    }
  }
  return NULL;
}

void
world_dc_event(User *user, World *w) {
  if (w->netstatus != WORLD_DISCONNECTED) {
    addEvent(user, w, EVENT_WORLD_DISCONNECT,
        "world:'%s',"
        "status:'disconnected',"
        "lines:%d,"
        "connectTime:%d,"
        "disconnectTime:%d",
        w->name,
        w->netlineCount,
        w->connectTime,
        time(NULL));
  }
}

// world_free does not use mutexes, any function calling it must be
// locked in with the user mutex.
void
world_free(World *w, int doEvents) {
  int i;
  printf("Closing world %s\n", w->name);
  // If the world is connected, then disconnect it. We'll queue a
  // disconnect event, and prevent the musocket thread from sending
  // an event itself.
  if (doEvents) {
    world_dc_event(w->user, w);
    addEvent(w->user, NULL, EVENT_WORLD_CLOSE,
        "world:'%s',"
        "status:'disconnected',"
        "lines:%d,"
        "openTime:%d,"
        "closeTime:%d,",
        w->name,
        w->lineCount,
        w->openTime,
        time(NULL));
  }
  net_disconnect(w);

  // Now clean up the world.
  for (i = 0; i < MAX_WORLD_EVENTS; i++) {
    if (w->events[i]) {
      event_free(w->events[i]);
    }
  }
  memset(w, 0, sizeof(World));
}

void
world_connect(User *user, char *worldname, char *host, char *port) {
  World *w;
  net_lock();
  pthread_mutex_lock(&user->mutex);
  w = world_get(user, worldname);
  if (w) {
    net_connect(w, host, port);
  } else {
    sysMessage(user, "No such world '%s'.", worldname);
  }
  pthread_mutex_unlock(&user->mutex);
  net_unlock();
}


void
world_disconnect(User *user, char *worldname) {
  World *w;
  net_lock();
  pthread_mutex_lock(&user->mutex);
  w = world_get(user, worldname);
  if (w && w->netstatus != WORLD_DISCONNECTED) {
    world_dc_event(user, w);
    net_disconnect(w);
  } else {
    sysMessage(user, "No such world '%s'.", worldname);
  }
  pthread_mutex_unlock(&user->mutex);
  net_unlock();
}

void
world_close(User *user, char *worldname) {
  World *w;
  net_lock();
  pthread_mutex_lock(&user->mutex);
  w = world_get(user, worldname);
  if (w) {
    world_free(w, 1);
  } else {
    sysMessage(user, "No such world '%s'.", worldname);
  }
  pthread_mutex_unlock(&user->mutex);
  net_unlock();
}

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
    if (!strcmp(user->worlds[i].name, worldname)) {
      sysMessage(user, "World '%s' already exists.", worldname);
    }
  }
  if (i >= MAX_USER_WORLDS) {
    for (i = 0; i < MAX_USER_WORLDS; i++) {
      if (user->worlds[i].name[0] == 0) {
        world = &user->worlds[i];
        // Allocate this world.
        memset(world, 0, sizeof(World));
        world->user = user;
        world->a2h.f = 'd';
        world->a2h.b = 'D';
        world->a2h.h = 0;
        world->a2h.u = 0;
        world->netstatus = WORLD_DISCONNECTED;
        snprintf(world->name, MAX_NAME_LEN, "%s", worldname);
        world->openTime = time(NULL);
        world->fd = -1;
        addEvent(user, NULL, EVENT_WORLD_OPEN,
            "world:'%s',"
            "status:'disconnected',"
            "lines:0,"
            "openTime:%d", world->name, world->openTime);
        break;
      }
    }
    if (i >= MAX_USER_WORLDS) {
      sysMessage(user, "Too many worlds.");
    }
  }
  pthread_mutex_unlock(&user->mutex);
}

void
world_echo(User *user, char *worldname, char *text) {
  World *w;
  char *jtext;
  pthread_mutex_lock(&user->mutex);
  w = world_get(user, worldname);
  if (w) {
    // Queue an on receive event.
    jtext = json_escape(text);
    addEvent(user, NULL, EVENT_WORLD_RECEIVE,
        "world:'%s',"
        "text:'%s'",
        w->name,
        jtext);
    w->lineCount++;
  } else {
    sysMessage(user, "No such world '%s'.", worldname);
  }
  pthread_mutex_unlock(&user->mutex);
}

void
world_send(User *user, char *worldname, char *text) {
  World *w;
  net_lock();
  pthread_mutex_lock(&user->mutex);
  w = world_get(user, worldname);
  if (w) {
    // Queue an on receive event.
    net_send(w, text);
  } else {
    sysMessage(user, "No such world '%s'.", worldname);
  }
  pthread_mutex_unlock(&user->mutex);
  net_unlock();
}
