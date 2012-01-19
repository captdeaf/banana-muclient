/** util.h
 *
 * Utility functions for Banana that are commonly used, but not directly
 * related to the logic/flow.
 */

#ifndef _MY_UTIL_H_
#define _MY_UTIL_H_

#define HTTP_FOUND "HTTP/1.1 302 Found\r\n"
#define HEADER_NOCACHE \
      "Expires: Thu, 01-Jan-1970 00:00:01 GMT\r\n" \
      "Cache: no-cache\r\n" \
      "max-age: 0\r\n" \
      "s-maxage: 0\r\n" \
      "Cache-Control: no-cache, no-store, must-revalidate\r\n"

void free_conndata(struct mg_connection *conn, struct mg_request_info *req);
void init_conndata(struct mg_connection *conn, struct mg_request_info *req);

// Does name start with an alphabetic character, does it contain only
// alphanumeric characters, underscores and periods? This destructively
// modifies name in place to be lower case.
int valid_name(char *name);

extern char *VAR_TOO_LONG;
// Returns NULL if not found, VAR_TOO_LONG if too long, and
// an allocated string (Which will be free()'d by free_post_data) if
// it exists.
char * get_qsvar(const struct mg_request_info *request_info,
              const char *name, int maxsize);

// Error headers
void send_404(struct mg_connection *conn);
void send_error(struct mg_connection *conn, const char *msg);

// Redirect a connection to an url.
void redirect_to(struct mg_connection *conn, const char *dest);
#define login_redirect(c) redirect_to(c, "index.html")

extern pthread_mutexattr_t pthread_recursive_attr; 

void do_noisy_lock(pthread_mutex_t *mutex, char *name,
                   char *fname, int linenum);
void do_noisy_unlock(pthread_mutex_t *mutex, char *name,
                     char *fname, int linenum);
void redirect_to_client(struct session *session,
                        struct user *user,
                        struct mg_connection *conn);

#define noisy_lock(m,n) do_noisy_lock(m,n,__FILE__,__LINE__)
#define noisy_unlock(m,n) do_noisy_unlock(m,n,__FILE__,__LINE__)

// Initialize util stuff.
void util_init();

#endif
