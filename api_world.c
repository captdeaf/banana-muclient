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

ACTION("world.open", api_world_open, API_DEFAULT) {
  char *world;
  getvar(world, "world", MAX_NAME_LEN);
  world_open(user, world);
}

ACTION("world.connect", api_world_connect, API_DEFAULT) {
  char *world, *host, *port;
  getvar(world, "world", MAX_NAME_LEN);
  getvar(host, "host", 120);
  getvar(port, "port", 20);
  world_connect(user, world, host, port);
}

ACTION("world.send", api_world_send, API_DEFAULT) {
  char *world;
  char *text;

  getvar(world, "world", MAX_NAME_LEN);
  getvar(text, "text", BUFFER_LEN * 3);

  world_send(user, world, text);
}

ACTION("world.echo", api_world_echo, API_DEFAULT) {
  char *world;
  char *text;

  getvar(world, "world", MAX_NAME_LEN);
  getvar(text, "text", BUFFER_LEN * 3);

  world_echo(user, world, text);
}

ACTION("world.disconnect", api_world_disconnect, API_DEFAULT) {
  char *world;
  getvar(world, "world", MAX_NAME_LEN);
  world_disconnect(user, world);
}

ACTION("world.close", api_world_close, API_DEFAULT) {
  char *world;
  getvar(world, "world", MAX_NAME_LEN);
  world_close(user, world);
}
