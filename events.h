/** events.h
 *
 * Defines events for Banana, including their worlds.
 */

#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

// JSONp is not a full JSON struct. It expects to be surrounded by {}s.
// This is so we can include seen. By using:
// "{ seen: %d, updateCount: %d, timestamp: %d, %s }", ..., json
typedef struct event {
  int refcount;
  // name is a constant, it's always one of EVENT_* from api.h
  const char *name;
  int updateCount;
  unsigned int seen;
  unsigned int timestamp;
  char *json;
} Event;

extern const char *update_header;

// Anything returned by json_escape needs to be free'd. */
char *json_escape(char *str);
void addEvent(struct user *user, struct world *world, const char *eventName,
              char *fmt, ...);
void messageEvent(struct user *user, struct world *world,
                  const char *eventName, const char *varname, char *fmt, ...);
#define sysMessage(user,fmt,args...) \
  messageEvent(user, NULL, EVENT_SYSMESSAGE, "message", fmt , ## args)

void event_wait(struct user *user, struct mg_connection *conn, int updateCount);

#define EVENT_SYSMESSAGE           "onSystemMessage"

#define EVENT_WORLD_OPEN           "world.onOpen"
#define EVENT_WORLD_CLOSE          "world.onClose"
#define EVENT_WORLD_RECEIVE        "world.onReceive"
#define EVENT_WORLD_CONNECT        "world.onConnect"
#define EVENT_WORLD_DISCONNECT     "world.onDisconnect"
#define EVENT_WORLD_CONNFAIL       "world.onConnectFail"
#define EVENT_WORLD_DISCONNFAIL    "world.onDisconnectFail"

#define EVENT_FILE_WRITE           "file.onWrite"
#define EVENT_FILE_WRITE_FAIL      "file.onWriteFail"
#define EVENT_FILE_DELETE_FAIL     "file.onDeleteFail"

#endif
