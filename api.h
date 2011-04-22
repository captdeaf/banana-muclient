/** api.h
 *
 * useful stuff for API calls
 */

#ifndef _MY_API_H_
#define _MY_API_H_

void write_ajax_header(struct mg_connection *);

#define nouse __attribute__ ((__unused__))
#define ACTION(s, n) \
  void n(User *user nouse, struct mg_connection *conn nouse, \
         const struct mg_request_info *req nouse)

typedef void (*actioncallback)(User *, struct mg_connection *,
                               const struct mg_request_info *);

typedef struct _action_list {
  const char *name;
  actioncallback handler;
} ActionList;

#endif

