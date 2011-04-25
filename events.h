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

void event_free(Event *event);

// Anything returned by json_escape needs to be free'd. */
char *json_escape(char *str);
void queueEvent(struct user *user, struct world *world, int alarm,
                const char *eventName, char *fmt, ...);
#define addEvent(u,w,e,f,args...) \
    queueEvent(u,w,1,e,f , ## args)
void messageEvent(struct user *user, struct world *world,
                  const char *eventName, const char *varname, char *fmt, ...);
#define sysMessage(user,fmt,args...) \
  messageEvent(user, NULL, EVENT_SYSMESSAGE, "message", fmt , ## args)

// This is used only for received events.
void addLines(struct user *user, struct world *world,
              char *lines[], int lineCount);

void event_startup(struct user *user, struct mg_connection *conn);
void event_wait(struct user *user, struct mg_connection *conn, int updateCount);

#define EVENT_SYSMESSAGE           "onSystemMessage"
#define EVENT_OVERFLOW             "onOverflow"

#define EVENT_WORLD_OPEN           "world.onOpen"
#define EVENT_WORLD_CLOSE          "world.onClose"
#define EVENT_WORLD_RECEIVE        "world.onReceive"
#define EVENT_WORLD_PROMPT         "world.onPrompt"
#define EVENT_WORLD_CONNECTING     "world.onConnecting"
#define EVENT_WORLD_CONNECT        "world.onConnect"
#define EVENT_WORLD_ERROR          "world.onError"
#define EVENT_WORLD_DISCONNECT     "world.onDisconnect"
#define EVENT_WORLD_CONNFAIL       "world.onConnectFail"
#define EVENT_WORLD_DISCONNFAIL    "world.onDisconnectFail"

#define EVENT_FILE_WRITE           "file.onWrite"
#define EVENT_FILE_WRITE_FAIL      "file.onWriteFail"
#define EVENT_FILE_DELETE_FAIL     "file.onDeleteFail"

#endif
