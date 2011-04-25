/* banana.h
 */

#ifndef _MY_BANANA_H_
#define _MY_BANANA_H_

#define _unused_ __attribute__ ((__unused__))

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "mongoose.h"
#include "logger.h"
#include "conf.h"
#include "sessions.h"
#include "worlds.h"
#include "users.h"
#include "banana.h"
#include "util.h"
#include "api.h"
#include "file.h"
#include "events.h"
#include "net.h"

#endif
