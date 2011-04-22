/** api.h
 *
 * useful stuff for API calls
 */

#ifndef _MY_API_H_
#define _MY_API_H_

void write_ajax_header(struct mg_connection *);

#define ACTION(s, n) \
  void n(User *user, struct mg_connection *conn, \
         const struct mg_request_info *req)

typedef void (*actioncallback)(User *, struct mg_connection *,
                               const struct mg_request_info *);

typedef struct _action_list {
  const char *name;
  actioncallback handler;
} ActionList;

#endif
