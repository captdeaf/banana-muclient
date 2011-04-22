/** api.c
 *
 * Handler stuff for api_* functions.
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
#include "api.h"
#include "file.h"

void write_ajax_header(struct mg_connection *conn) {
  static const char *header =
      "HTTP/1.1 200 OK\r\n"
      "Cache: no-cache\r\n"
      "Content-Type: application/x-javascript\r\n"
      "\r\n";
  mg_write(conn, header, strlen(header));
}
