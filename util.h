/** util.h
 *
 * Utility functions for Banana that are commonly used, but not directly
 * related to the logic/flow.
 */

#ifndef _MY_UTIL_H_
#define _MY_UTIL_H_

#define HTTP_FOUND "HTTP/1.1 302 Found\r\n"

typedef struct post_data PostData;
void free_post_data(PostData *fd);
PostData *read_post_data(struct mg_connection *conn);

// Does name start with an alphabetic character, does it contain only
// alphanumeric characters, underscores and periods? This destructively
// modifies name in place to be lower case.
int valid_name(char *name);

// Returns 1 if found, 0 if not.
int get_qsvar(const struct mg_request_info *request_info,
              const char *name, char *dst, size_t dst_len);

// Redirect a connection to an url.
void redirect_to(struct mg_connection *conn, const char *dest);
#define login_redirect(c) redirect_to(c, "/index.html")

extern pthread_mutexattr_t pthread_recursive_attr; 

// Initialize util stuff.
void util_init();

#endif
