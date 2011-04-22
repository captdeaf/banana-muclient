
/* mg_session.c
 *
 * Session handling for banana.
 *
 * Ripped shamelessly from Mongoose chat example. Thank you, Mongoose!
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <limits.h>
#include <ctype.h>

#include "mongoose.h"
#include "util.h"
#include "banana.h"
#include "conf.h"

pthread_mutexattr_t pthread_recursive_attr; 

struct post_data {
  int size;
  char *value;
};

unsigned char valid_chars[0x100];

void
util_init() {
  int i;
  pthread_mutexattr_init(&pthread_recursive_attr); 
  pthread_mutexattr_settype(&pthread_recursive_attr, PTHREAD_MUTEX_RECURSIVE); 
  for (i = 0; i < 0x100; i++) {
    if (isalpha(i)) {
      valid_chars[i] = tolower(i);
    } else if (isdigit(i) || i == '_' || i == '.') {
      valid_chars[i] = i;
    } else {
      valid_chars[i] = 0;
    }
  }
}

int
valid_name(char *name) {
  char *ptr;
  if (!name || !*name) return 0;
  if (!isalpha(*name)) return 0;
  for (ptr = name; *ptr; ptr++) {
    if (valid_chars[(unsigned char) *ptr]) {
      *ptr = valid_chars[(unsigned char) *ptr];
    } else {
      return 0;
    }
  }
  return (ptr - name) < (MAX_NAME_LEN - 1);
}

int
get_qsvar(const struct mg_request_info *req,
          const char *name, char *dst, size_t dst_len) {
  const char *qs = req->query_string;
  PostData *fd = (PostData *) req->user_data;

  if (!dst || dst_len < 1) return 0;
  if (qs == NULL && req->user_data == NULL) { return 0; }


  if (qs) {
    if (mg_get_var(qs, strlen(qs), name, dst, dst_len) >= 0) {
      return 1;
    }
  }
  if (req->user_data) {
    if (mg_get_var(fd->value, fd->size, name, dst, dst_len) >= 0) {
      return 1;
    }
  }
  *dst = '\0';
  return 0;
}

// Redirect user to the login form. In the cookie, store the original URL
// we came from, so that after the authorization we could redirect back.
void
redirect_to(struct mg_connection *conn, const char *dest) {
  mg_printf(conn, "HTTP/1.1 302 Found\r\n"
            "Location: %s\r\n\r\n",
            dest);
}

void
free_post_data(PostData *pd) {
  if (pd) {
    if (pd->value) {
      free(pd->value);
    }
    free(pd);
  }
}

// TODO: Multipart, files, etc.
// Break when Content-Type header != "application/x-www-form-urlencoded"
PostData *
read_post_data(struct mg_connection *conn) {
  char *buff;
  PostData *pd;
#define BUFF_LEN 1024
  int size = BUFF_LEN;
  int sofar = 0;
  const char *ctype = mg_get_header(conn, "Content-Type");

  // If it's not application/x-www-form-urlencoded, we don't know what to do,
  // so just return NULL for now =).
  if (!ctype) {
    printf("Ruh roh, No Content-type for a post?");
    return NULL;
  }
  if (strncmp(ctype, "application/x-www-form-urlencoded", 23)) {
    printf("Ruh roh, I got a connection type, '%s' that I don't know what to do with!",
           ctype);
    return NULL;
  }

  buff = malloc(size);
  sofar = mg_read(conn, buff, BUFF_LEN);
  while (sofar == size && size < MAX_FILE_SIZE) {
    size += BUFF_LEN;
    buff = realloc(buff, size);
    mg_read(conn, buff + sofar, BUFF_LEN);
  }

  if (sofar == 0) {
    free(buff);
    return NULL;
  }

  pd = malloc(sizeof(PostData));
  pd->size = sofar;
  pd->value = buff;

  return pd;
}
