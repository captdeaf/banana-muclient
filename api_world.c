/** api_world.c
 *
 * /action/world.foo api calls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "mongoose.h"
#include "conf.h"
#include "sessions.h"
#include "worlds.h"
#include "users.h"
#include "banana.h"
#include "util.h"
#include "events.h"
#include "api.h"

ACTION("world.open", api_world_open) {
  char world[MAX_NAME_LEN];
  if (get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    world_open(user, world);
  } else {
    sysMessage(user, "API error: world not passed to open?");
  }
  write_ajax_header(conn);
}

ACTION("world.connect", api_world_connect) {
}

ACTION("world.send", api_world_send) {
}

ACTION("world.echo", api_world_echo) {
}

ACTION("world.disconnect", api_world_disconnect) {
}

ACTION("world.close", api_world_close) {
}
