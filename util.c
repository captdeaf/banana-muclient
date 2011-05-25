/* util.c
 *
 * http-related and other utility functions for Banana.
 */

#include "banana.h"

pthread_mutexattr_t pthread_recursive_attr; 

#define NOISY_LOCK

#ifdef NOISY_LOCK
Logger *threadlog;
#endif

void
do_noisy_lock(pthread_mutex_t *mutex,
              char *name _unused_,
              char *fname _unused_,
              int linenum _unused_) {
#ifdef NOISY_LOCK
  llog(threadlog, "Mutex '%s' try_lock by %X at %s:%d",
       name, pthread_self(), fname, linenum);
#endif
  pthread_mutex_lock(mutex);
#ifdef NOISY_LOCK
  llog(threadlog, "Mutex '%s' locked by %X at %s:%d",
       name, pthread_self(), fname, linenum);
#endif
}

void
do_noisy_unlock(pthread_mutex_t *mutex,
                char *name _unused_,
                char *fname _unused_,
                int linenum _unused_) {
#ifdef NOISY_LOCK
  llog(threadlog, "Mutex '%s' unlocked by %X at %s:%d",
       name, pthread_self(), fname, linenum);
#endif
  pthread_mutex_unlock(mutex);
}

#define MAX_PARAMS 40

struct param {
  const char *name;
  char *val;
};

typedef struct post_data {
  int size;
  char *value;
  struct param params[MAX_PARAMS];
  int    nparams;
} PostData;

unsigned char valid_chars[0x100];

void
util_init() {
  int i;
#ifdef NOISY_LOCK
  threadlog = logger_new("logs/threads");
#endif
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

char *VAR_TOO_LONG = "#-1 PARAMETER WAS TOO LONG";
char *
get_qsvar(const struct mg_request_info *req, const char *name, int maxlen) {
  const char *qs = req->query_string;
  int i;
  int ret;
  char buff[MAX_FILE_SIZE];
  PostData *pd = (PostData *) req->user_data;

  if (maxlen > MAX_FILE_SIZE) {
    printf("maxlen in get_qsvar is too high: %d.\n", maxlen);
    maxlen = MAX_FILE_SIZE;
  }

  for (i = 0; i < pd->nparams; i++) {
    if (!strcmp(pd->params[i].name, name)) {
      return pd->params[i].val;
    }
  }
  if (i >= MAX_PARAMS) {
    return strdup("MAX PARAMS REACHED");
  }

  if (qs == NULL && req->user_data == NULL) { return NULL; }

  ret = -1;
  if (qs) {
    ret = mg_get_var(qs, strlen(qs), name, buff, maxlen);
  }
  if (ret == -1 && req->user_data) {
    ret = mg_get_var(pd->value, pd->size, name, buff, maxlen);
  }
  if (ret >= 0) {
    pd->params[i].name = name;
    pd->params[i].val = strdup(buff);
    pd->params[i].val[ret] = '\0';
    pd->nparams++;
    return pd->params[i].val;
  } else if (ret == -2) {
    return VAR_TOO_LONG;
  }
  return NULL;
}

// Redirect user to their preferred client.
void
redirect_to_client(struct session *session,
                   struct user *user,
                   struct mg_connection *conn) {
  char clfile[MAX_PATH_LEN];
  char *client;
  snprintf(clfile, MAX_PATH_LEN, "%s/%s", user->dir, "client");
  client = file_read(clfile);
  if (client) {
    strchomp(client);
    snprintf(clfile, MAX_PATH_LEN, "/%s", client);
    free(client);
  } else {
    snprintf(clfile, MAX_PATH_LEN, "/webfugue");
  }
  if (session && session->cookie_string) {
    mg_printf(conn, "HTTP/1.1 302 Found\r\n"
              HEADER_NOCACHE
              "%s\r\n"
              "Location: %s\r\n\r\n",
              session->cookie_string,
              clfile);
  } else {
    mg_printf(conn, "HTTP/1.1 302 Found\r\n"
              HEADER_NOCACHE
              "Location: %s\r\n\r\n",
              clfile);
  }
}

void
redirect_to(struct mg_connection *conn, const char *dest) {
  mg_printf(conn, "HTTP/1.1 302 Found\r\n"
            HEADER_NOCACHE
            "Location: %s\r\n\r\n",
            dest);
}

void
free_conndata(struct mg_connection *conn _unused_, struct mg_request_info *req) {
  int i;
  PostData *pd = (PostData *) req->user_data;

  if (pd) {
    for (i = 0; i < pd->nparams; i++) {
      free(pd->params[i].val);
    }
    if (pd->value) {
      free(pd->value);
    }
    free(pd);
  }
}

// TODO: Multipart, files, etc.
// Break when Content-Type header != "application/x-www-form-urlencoded"
void
init_conndata(struct mg_connection *conn, struct mg_request_info *req) {
  char *buff;
  PostData *pd;
#define BUFF_LEN 2048
  int size = BUFF_LEN;
  int sofar = 0;
  const char *ctype = mg_get_header(conn, "Content-Type");

  // Allocate a postData at all times.
  pd = malloc(sizeof(PostData));
  pd->nparams = 0;
  pd->size = 0;
  pd->value = NULL;

  req->user_data = (void *) pd;

  // If it's a POST, then read in request methods.
  if (!strcasecmp(req->request_method,"POST")) {
    // If it's not application/x-www-form-urlencoded, we don't know what to do,
    // so just return NULL for now =).
    if (!ctype) { printf("Ruh roh, No Content-type for a post?"); return; }

    if (strncmp(ctype, "application/x-www-form-urlencoded", 23)) {
      printf("Ruh roh, I got a content-type, '%s' that I don't know what to do with!",
             ctype);
      return;
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
      return;
    }

    pd->size = sofar;
    pd->value = buff;

  }
}

void
send_404(struct mg_connection *conn) {
  char *msg = "404: File not found.";
  mg_printf(conn,
      "HTTP/1.1 404 Not Found"
      HEADER_NOCACHE
      "Content-Type: text/plain\r\n"
      "Content-Length: %d\r\n\r\n"
      "%s",
      strlen(msg), msg
      );
}

void
send_error(struct mg_connection *conn, const char *msg) {
  mg_printf(conn,
      "HTTP/1.1 500 Internal Server Error\r\n"
      HEADER_NOCACHE
      "Content-Type: text/plain\r\n"
      "Content-Length: %d\r\n\r\n"
      "%s",
      strlen(msg), msg
      );
}
