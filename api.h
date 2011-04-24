/** api.h
 *
 * useful stuff for API calls
 */

#ifndef _MY_API_H_
#define _MY_API_H_

void write_ajax_header(struct mg_connection *);

#define nouse __attribute__ ((__unused__))
#define ACTION(s, n, f) \
  void n(struct user *user nouse, struct mg_connection *conn nouse, \
         const struct mg_request_info *req nouse, const char *called_as nouse)

typedef void (*actioncallback)(struct user *, struct mg_connection *,
                               const struct mg_request_info *,
                               const char *);

// Bitwise fields for ACTION() / action_list
#define API_DEFAULT    0x00
#define API_NOHEADER   0x01
#define API_POSTONLY   0x02

typedef struct _action_list {
  const char *name;
  actioncallback handler;
  int   flags;
} ActionList;

// getvar() is only valid within the context of an ACTION()
#define getvar(val,name,len) \
  do { \
    val = get_qsvar(req, name, len); \
    if (val == VAR_TOO_LONG) { \
      sysMessage(user, "API error: %s: '%s' too long.", called_as, name); \
      return; \
    } else if (val == NULL) { \
      sysMessage(user, "API error: %s: '%s' not provided.", called_as, name); \
      return; \
    } \
  } while (0)

extern ActionList allActions[];

#endif /* _API_INC_H_ */
