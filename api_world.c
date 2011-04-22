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
#include "users.h"
#include "banana.h"
#include "util.h"
#include "api.h"

ACTION("world.open", world_open) {
}

ACTION("world.connect", world_connect) {
}

ACTION("world.send", world_send) {
}

ACTION("world.echo", world_echo) {
}

ACTION("world.disconnect", world_disconnect) {
}

ACTION("world.close", world_close) {
}
