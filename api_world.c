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
  char world[MAX_NAME_LEN];
  char host[BUFFER_LEN];
  char port[BUFFER_LEN];
  if (!get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    sysMessage(user, "API error: world not passed to connect?");
    return;
  }
  if (!get_qsvar(req, "host", host, BUFFER_LEN)) {
    sysMessage(user, "API error: host not passed to connect?");
    return;
  }
  if (!get_qsvar(req, "port", port, BUFFER_LEN)) {
    sysMessage(user, "API error: port not passed to connect?");
    return;
  }
  world_connect(user, world, host, port);
  write_ajax_header(conn);
}

ACTION("world.send", api_world_send) {
  char world[MAX_NAME_LEN];
  char text[BUFFER_LEN];
  if (!get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    sysMessage(user, "API error: world not passed to send?");
    return;
  }
  if (!get_qsvar(req, "text", text, BUFFER_LEN)) {
    sysMessage(user, "API error: text not passed to send?");
    return;
  }
  world_send(user, world, text);
  write_ajax_header(conn);
}

ACTION("world.echo", api_world_echo) {
  char world[MAX_NAME_LEN];
  char text[BUFFER_LEN];
  if (!get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    sysMessage(user, "API error: world not passed to echo?");
    return;
  }
  if (!get_qsvar(req, "text", text, BUFFER_LEN)) {
    sysMessage(user, "API error: text not passed to echo?");
    return;
  }
  world_echo(user, world, text);
  write_ajax_header(conn);
}

ACTION("world.disconnect", api_world_disconnect) {
  char world[MAX_NAME_LEN];
  if (get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    world_disconnect(user, world);
  } else {
    sysMessage(user, "API error: world not passed to close?");
  }
  write_ajax_header(conn);
}

ACTION("world.close", api_world_close) {
  char world[MAX_NAME_LEN];
  if (get_qsvar(req, "world", world, MAX_NAME_LEN)) {
    world_close(user, world);
  } else {
    sysMessage(user, "API error: world not passed to close?");
  }
  write_ajax_header(conn);
}
