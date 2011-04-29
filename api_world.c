/** api_world.c
 *
 * /action/world.foo api calls.
 */

#include "banana.h"

ACTION("world.open", api_world_open, API_DEFAULT) {
  char *world;
  getvar(world, "world", MAX_NAME_LEN);
  if (user->isGuest) {
    // Guest users can only have one world, named "world",
    // and may only connect to one host/port, specified in their
    // host file as "host port"
    if (strcmp(world, "world")) {
      sysMessage(user, "Guest users can only have one world, named 'world'");
      return;
    }
  }
  world_open(user, world);
}

ACTION("world.connect", api_world_connect, API_DEFAULT) {
  char *world, *host, *port;
  char worldtest[300];
  char hostpath[300];
  char *hostlimit;
  int ret;
  getvar(world, "world", MAX_NAME_LEN);
  getvar(host, "host", 120);
  getvar(port, "port", 20);
  if (user->isGuest) {
    // Guest users can only have one world, named "world",
    // and may only connect to one host/port, specified in their
    // host file as "host port"
    if (strcmp(world, "world")) {
      messageEvent(user, NULL, EVENT_WORLD_CONNFAIL, "cause",
          "Guest users can only have one world, named 'world'");
      return;
    }
    snprintf(hostpath, 300, "%s/host", user->dir);
    hostlimit = file_read(hostpath);
    if (!hostlimit) {
      messageEvent(user, NULL, EVENT_WORLD_CONNFAIL, "cause",
          "Guest users can only connect to a specified host port, but one is "
          " not provided on the server.");
      return;
    }
    snprintf(worldtest, 300, "%s %s\n", host, port);
    ret = !strcmp(worldtest, hostlimit);
    free(hostlimit);
    if (!ret) {
      messageEvent(user, NULL, EVENT_WORLD_CONNFAIL, "cause",
          "The host+port does not match the allowed one for this guest user.");
      return;
    }
  }
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
