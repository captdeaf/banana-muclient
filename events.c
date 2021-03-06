/* events.h
 *
 * User handling for banana.
 */

#include "banana.h"

/*
typedef struct event {
  // json is not a full JSON struct. It expects to be surrounded by {}s.
  // This is so we can include seen. By using:
  // "{ seen: %d, updateCount: %d, %s }", seen, updateCount, json
  int seen;
  int updateCount;
  char *json;
} Event;
*/
const char *update_header = 
        "HTTP/1.1 200 OK\r\n"
        HEADER_NOCACHE
        "Connection: close\r\n"
        "Content-Type: application/x-javascript; charset=utf-8\r\n"
        "updateCount: %d\r\n"
        "\r\n";

void
event_free(Event *event) {
  event->refcount--;
  if (event->refcount <= 0) {
    free(event->json);
    free(event);
  }
}

void
addLines(struct user *user, struct world *world,
         char *lines[], int nlines) {
  int i;
  noisy_lock(&user->mutex, user->name);
  for (i = 0; i < nlines; i++) {
    // lines are already escaped in ansi2html
    queueEvent(user, world, 0, EVENT_WORLD_RECEIVE,
               "world:'%s',"
               "text:'%s'",
               world->name,
               lines[i]
               );
    llog(world->logger, "%s", remove_markup(lines[i]));
  }
  pthread_cond_broadcast(&user->evtAlarm);
  noisy_unlock(&user->mutex, user->name);
}

void
queueEvent(struct user *user, struct world *world, int alarm,
           const char *eventName, char *fmt, ...) {
  Event *event = malloc(sizeof(Event));
  va_list args;
  int ret;

  noisy_lock(&user->mutex, user->name);

  event->timestamp = time(NULL);
  event->seen = 0;
  event->refcount = 0;
  event->name = eventName;
  event->updateCount = user->updateCount++;

  va_start(args, fmt);
  ret = vasprintf(&event->json, fmt, args);
  va_end(args);
  if (ret < 0) {
    // Fatal error, braincell 5. abort! abort! We won't do jack.
    free(event);
    noisy_unlock(&user->mutex, user->name);
    return;
  }

  if (user != NULL) {
    if (user->events[user->evtE]) {
      // We've gone full round robin.
      event_free(user->events[user->evtE]);
      user->evtS++;
      user->evtS %= MAX_USER_EVENTS;
    }
    event->refcount++;
    user->events[user->evtE] = event;
    // Increment evtE
    user->evtE++;
    user->evtE %= MAX_USER_EVENTS;
  }

  if (world != NULL) {
    if (world->events[world->evtE]) {
      // Bump a world event.
      event_free(world->events[world->evtE]);
      world->evtS++;
      world->evtS %= MAX_WORLD_EVENTS;
    }
    event->refcount++;
    world->events[world->evtE] = event;
    world->evtE++;
    world->evtE %= MAX_WORLD_EVENTS;
  }

  // Alarm, alarm , alarm, alarm. We have an alarm, people. Does anyone see
  // it? There's an alarm. I said it's an alarm. Alarm.

  if (alarm) {
    pthread_cond_broadcast(&user->evtAlarm);
  }
  noisy_unlock(&user->mutex, user->name);
}

char *
json_escape(char *str) {
  // We prepare for the worst, since even the worst isn't very bad.
  int len = strlen(str) + 1;
  char *ret = malloc(len * 4);
  char *r;

  if (!ret) return strdup("JSON_ESCAPE IS OUT OF MEMORY");

  for (r = ret; *str; str++) {
    if (*str == '\n') {
      *(r++) = '\\';
      *(r++) = 'n';
    } else if (*str == '\t') {
      *(r++) = '\\';
      *(r++) = 't';
    } else if (*str == '\r') {
      *(r++) = '\\';
      *(r++) = 'r';
    } else if (!isprint(*str)) {
      r += sprintf(r, "\\x%.2x", (unsigned char) *str);
    } else if (*str == '\\') {
      *(r++) = '\\';
      *(r++) = '\\';
    } else if (*str == '\"') {
      *(r++) = '\\';
      *(r++) = '\"';
    } else if (*str == '\'') {
      *(r++) = '\\';
      *(r++) = '\'';
    } else {
      *(r++) = *str;
    }
  }
  *(r) = '\0';
  return ret;
}

// Send a simple message event. This is _not_ added to the world,
// but if world is provided, then its name is used.
void
messageEvent(User *user, World *world, const char *eventName,
             const char *varname, char *fmt, ...) {
  va_list args;
  char *msg, *jmsg;

  va_start(args, fmt);
  vasprintf(&msg, fmt, args);
  va_end(args);

  jmsg = json_escape(msg);
  free(msg);

  // If it's a world-related event, add world name.
  if (world) {
    addEvent(user, NULL, eventName,
        "world: '%s', %s: '%s'", world->name, varname, jmsg);
  } else {
    addEvent(user, NULL, eventName, "%s: '%s'", varname, jmsg);
  }
  free(jmsg);
}

static void
dump_overflow_event(int count, struct mg_connection *conn) {
  mg_printf(conn,
      "API.triggerEvent({"
        "eventname:'%s',"
        "updateCount:-1,"
        "seen:1,"
        "timestamp:%d,"
        "count:%d});\r\n",
        EVENT_OVERFLOW,
        time(NULL),
        count);
}

static void
dump_event(Event *event, struct mg_connection *conn) {
  event->seen++;

  mg_printf(conn,
      "API.triggerEvent({"
        "eventname:'%s',"
        "updateCount:%d,"
        "seen:%d,"
        "timestamp:%d,"
        "%s});\r\n",
        event->name,
        event->updateCount,
        event->seen,
        event->timestamp,
        event->json);
}

void
event_startup(struct user *user, struct mg_connection *conn) {
  int i, j, e;
  World *w;
  noisy_lock(&user->mutex, user->name);

  // Header: updateCount
  mg_printf(conn, update_header, user->updateCount);

  // Set usernames.
  mg_printf(conn,
      "API.setNames('%s','%s');",
      user->loginname, user->name);

  // All open worlds.
  for (i = 0; i < MAX_USER_WORLDS; i++) {
    w = &user->worlds[i];
    if (w->name[0]) {
      // Send the WORLD_OPEN event.
      mg_printf(conn,
          "API.triggerEvent({"
            "eventname:'%s',"
            "updateCount:-1,"
            "seen:0,"
            "timestamp:%d,"
            "world:'%s',"
            "status:'%s',"
            "lines:%d,"
            "openTime:%d});\r\n",
            EVENT_WORLD_OPEN,
            w->openTime,
            w->name,
            world_status(w),
            w->lineCount,
            w->openTime);
    }
  }

  for (i = 0; i < MAX_USER_WORLDS; i++) {
    w = &user->worlds[i];
    if (w->name[0]) {
      e = w->evtS;
      for (j = 0; j < MAX_WORLD_EVENTS; j++) {
        if (w->events[e]) {
          dump_event(w->events[e], conn);
        } else {
          break;
        }
        e++;
        e %= MAX_WORLD_EVENTS;
      }
    }
  }

  noisy_unlock(&user->mutex, user->name);
}

void
event_wait(struct user *user, struct mg_connection *conn, int updateCount) {
  struct timespec to;
  int count, i;

  noisy_lock(&user->mutex, user->name);

  if (user->updateCount <= updateCount) {
    // Nuts, no updates. Let's take a nap.
    clock_gettime(CLOCK_REALTIME, &to);
    to.tv_sec += LONG_POLL_SECONDS;
    to.tv_nsec = 0;
    pthread_cond_timedwait(&(user->evtAlarm), &(user->mutex), &to);
  }
  if (user->updateCount > updateCount) {
    // We got something, Kit!
    // Now we gotta dump all the events to conn. Happy happy joy joy.
    mg_printf(conn, update_header, user->updateCount);

    // Find out how many events behind the user is.
    count = user->updateCount - updateCount;
    if (count > MAX_USER_EVENTS) {
      dump_overflow_event(count - MAX_USER_EVENTS, conn);
      count = MAX_USER_EVENTS;
    }

    // Find the starting location.
    i = user->evtE - count;
    if (i < 0) { i += MAX_USER_EVENTS; }
    while (count-- > 0) {
      dump_event(user->events[i], conn);
      i += 1;
      i %= MAX_USER_EVENTS;
    }
  } else {
    // We don't got shit!
    mg_printf(conn, update_header, user->updateCount);
  }

  noisy_unlock(&user->mutex, user->name);
}
