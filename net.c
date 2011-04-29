/* net.c
 *
 * Network library for connecting to mu*s.
 */

#include "banana.h"

#include "telnet.h"
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <iconv.h>

#define MAX_EVENTS 20

static int epfd;
static pthread_mutex_t sockmutex;
static pthread_key_t pt_key;

struct new_conn_info {
  // Host is strdup'd, so free.
  char *host;
  char *port;
  World *world;
};

inline void
net_lock() {
  noisy_lock(&sockmutex, "sock");
}

inline void
net_unlock() {
  noisy_unlock(&sockmutex, "sock");
}

void
net_sigusr1(int sig _unused_) {
  int *fd;
  struct epoll_event epvt;
  fd = pthread_getspecific(pt_key);
  if (fd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, *fd, &epvt);
    close(*fd);
  }
}

static void *
get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// TODO: Figure out locking between threads? Or trust the other threads
// to kill this one when appropriate?
// Thank you, Beej, for your awesome code and tutorial.
static void *
thread_connect(void *arg) {
  struct new_conn_info *nci = (struct new_conn_info *) arg;
  char host[200];
  char port[20];
  World *world;
  struct epoll_event epvt;

  int sockfd;
  int rv;
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Copy over from nci.
  snprintf(host, 200, "%s", nci->host);
  snprintf(port, 20, "%s", nci->port);
  world = nci->world;

  // Free nci
  free(nci->host);
  free(nci->port);
  free(nci);

  rv = getaddrinfo(host, port, &hints, &servinfo);

  if (rv != 0) {
    messageEvent(world->user, world, EVENT_WORLD_CONNFAIL, "cause",
                 "Connect failed: '%s'.", gai_strerror(rv));
    world->netstatus = WORLD_DISCONNECTED;
    return NULL;
  }

  // Constantly check if we stop connecting. Heh. Should only happen
  // on a close or disconnect, so no events.
  messageEvent(world->user, world, EVENT_WORLD_CONNECTING, "status",
               "Resolved host.");

  // loop through all the results and connect to the first we can
  net_lock();
  for(p = servinfo; p != NULL; p = p->ai_next) {
    // Make a socket for the given type.
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
      // perror("client: socket");
      continue;
    }

    // Connect it.
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      // perror("client: connect");
      continue;
    }

    break;
  }
  if (p && sockfd >= 0) {
    pthread_setspecific(pt_key, &sockfd);
  }
  net_unlock();

  if (p == NULL) {
    messageEvent(world->user, world, EVENT_WORLD_CONNFAIL, "cause",
                 "Failed to connect.");
    world->netstatus = WORLD_DISCONNECTED;
    return NULL;
  }

  // Copy the ipaddr over.
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
      world->ipaddr, 49);

  // Set the sockfd opts we need: Nonblock 
  if (1) {
    struct linger linger;
    int flags;
   
    // NONBLOCKING
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // LINGERING
    linger.l_onoff = 1;
    linger.l_linger = 5;
    setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
  }

  net_lock();

  world->fd = sockfd;
  // Now to add this to epoll.
  epvt.data.ptr = world;
  epvt.events = EPOLLIN | EPOLLERR;

  epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &epvt);

  world->connectTime = time(NULL);

  llog(world->logger, "-- WORLD CONNECTED to '%s' port '%s' --", host, port);
  slog("Connected world '%s' for user '%s'", world->name, world->user->name);

  net_unlock();

  world->netstatus = WORLD_CONNECTED;
  messageEvent(world->user, world, EVENT_WORLD_CONNECT, "ipaddr",
               "%s", world->ipaddr);

  // Clear this thread from world.
  memset(&world->connthread, 0, sizeof(pthread_t));

  // And we're done =).
  return NULL;
}

void
net_connect(World *world, char *host, char *port) {
  struct new_conn_info *nci;

  // First, ensure it's not already connecting.
  if (world->netstatus != WORLD_DISCONNECTED) {
    messageEvent(world->user, world, EVENT_WORLD_CONNFAIL, "cause",
                 "Socket for '%s' already exists.", world->name);
  } else {
    nci = malloc(sizeof(struct new_conn_info));
    nci->port = strdup(port);
    nci->host = strdup(host);
    nci->world = world;
    world->netstatus = WORLD_CONNECTING;

    // And spawn a new thread to do the connection.
    pthread_create(&world->connthread, NULL, thread_connect, (void *) nci);
    messageEvent(world->user, world, EVENT_WORLD_CONNECTING, "status",
                 "Beginning connect.");
  }
}

// #define DEBUG_NET
#ifdef DEBUG_NET
static int writing = -1;
static void
write_dbg(int fd, unsigned char *text, int len) {
  int i;
  char c;
  // Debug prints.
  for (i = 0; i < len; i++) {
    c = text[i] & 0xFF;
    if (writing != 1) { printf("\n* "); writing = 1; }
    printf("%.2X ", c & 0xff);
  }
  send(fd, text, len, MSG_DONTWAIT);
}

int
recv_debug(int fd, unsigned char *ptr, int size, int flags) {
  int ret = recv(fd, ptr, size, flags);
  int i;
  char c;
  for (i = 0; i < ret; i++) {
    c = ptr[i] & 0xFF;
    if (writing != 0) { printf("\n"); writing = 0; }
    printf("%.2X ", c & 0xff);
  }
  return ret;
}
#define recv_raw(f, p, c, fl) recv_debug(f, (unsigned char *) p, c, fl)
#define write_raw(f,t,l) write_dbg(f,(unsigned char *) t,l)
#else
#define recv_raw(f, p, c, fl) recv(f, p, c, fl)
#define write_raw(f,t,l) send(f,(unsigned char *) t,l, MSG_DONTWAIT)
#endif

int write_stoppers[0x100];

void
write_escaped(int fd, char *text, int len) {
  int start = 0;
  int end = 0;
  unsigned char *s = (unsigned char *) text;

  while (start < len) {
    if (s[start] == _IAC) {
      write_raw(fd, IAC, 1);
    } else if (s[start] == '\r') {
      // We skip over \rs, since we turn \ns into \r\n.
      start++;
      continue;
    } else if (s[start] == '\n') {
      write_raw(fd, "\r\n", 2);
      start++;
      continue;
    }
    for (end = start + 1; end < len && s[end] != _IAC; end++);
    write_raw(fd, s + start, end - start);
    start = end;
  }
}

void
net_send(World *world, char *text) {
  noisy_lock(&world->user->mutex, world->user->name);
  if (world->fd >= 0) {
    write_escaped(world->fd, text, strlen(text));
    write_raw(world->fd, "\r\n", 2);
  } else {
    messageEvent(world->user, world, EVENT_WORLD_ERROR, "cause",
                 "Not connected.");
  }
#ifdef DEBUG_NET
  printf("\n");
  writing = -1;
#endif
  noisy_unlock(&world->user->mutex, world->user->name);
}

void
net_disconnect(World *world) {
  int sockfd = world->fd;
  int status = world->netstatus;
  struct epoll_event epvt;
  pthread_t connthread = world->connthread;
  world->fd = -1;
  world->netstatus = WORLD_DISCONNECTED;
  world->connthread = 0;

  // 
  if (status == WORLD_CONNECTING && connthread) {
    pthread_kill(connthread, SIGUSR1);
  }
  if (sockfd >= 0) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &epvt);
    close(sockfd);
  }
}

int
net_init() {
  // Stuff for the new_connection-spawned threads.
  signal(SIGUSR1, net_sigusr1);
  pthread_key_create(&pt_key, NULL);

  memset(write_stoppers, 0, sizeof(write_stoppers));
  write_stoppers['\r'] = 1;
  write_stoppers['\n'] = 1;
  write_stoppers[_IAC] = 1;

  pthread_mutex_init(&sockmutex, &pthread_recursive_attr);
  epfd = epoll_create(10);
  if (epfd < 0) {
    perror("epoll_create");
    return 1;
  }
  return 0;
}

// Remove HTML markup from a line of text. */
char *
remove_markup(char *str) {
  char *r = str;
  char *s = str;
  while (s && *s) {
    switch (*s) {
    case '&':
      if (!strncmp(s, "&nbsp;", 6)) {
        s += 6;
        *(r++) = ' ';
      } else if (!strncmp(s, "&amp;", 5)) {
        s += 5;
        *(r++) = '&';
      } else if (!strncmp(s, "&gt;", 4)) {
        s += 4;
        *(r++) = '>';
      } else if (!strncmp(s, "&lt;", 4)) {
        s += 4;
        *(r++) = '<';
      }
      break;
    case '<':
      while (*s && *s != '>') s++;
      s++;
      break;
    case '\\':
      s++;
    default:
      *(r++) = *s++;
    }
  }
  *(r) = '\0';
  return str;
}

char *
convert_charset( char *from_charset, char *to_charset, char *input )
{
  size_t input_size, output_size, bytes_converted;
  char * output;
  char * ret;
  iconv_t cd;

  cd = iconv_open( to_charset, from_charset);
  if ( cd == (iconv_t) -1 ) {
    //Something went wrong with iconv_open
    if (errno == EINVAL)
    {
      slog("Conversion from %s to %s not available", from_charset, to_charset);
    }
    perror("iconv_open");
    return NULL;
  }

  input_size = strlen(input);
  output_size = 4 * input_size;
  ret = output = malloc(output_size + 1);
  memset(output, 0, output_size + 1);

  bytes_converted = iconv(cd, &input, &input_size, &output, &output_size);
  if ( iconv_close (cd) != 0) {
    perror("iconv_open");
    slog("Error closing iconv port?");
  }

  return ret;
}

// start_span and end_span must write out text that is JSON-safe.
int
start_span(char **r, struct a2h *ah) {
  int ret = 0;
  if (ah->flags) {
    ret = 1;
    if (ah->flags & ANSI_UNDERLINE) { *r += sprintf(*r, "<u>"); }
    if (ah->flags & ANSI_FLASH) { *r += sprintf(*r, "<em>"); }
    if (ah->flags & ANSI_HILITE) { *r += sprintf(*r, "<strong>"); }
    if (ah->flags & ANSI_INVERT) { *r += sprintf(*r, "<span class=\\\"invert\\\">"); }
  }
  if (ah->f[0] || ah->b[0]) {
    ret = 1;
    (*r) += sprintf(*r, "<span class=\\\"");
    if (ah->b[0]) {
      (*r) += sprintf(*r, "%s", ah->b);
    }
    if (ah->f[0]) {
      (*r) += sprintf(*r, "%s%s", ah->b[0] ? " " : "", ah->f);
    }
    (*r) += sprintf(*r, "\\\">");
  }
  return ret;
}

/** We need to spit out The bits we printed, in reverse */
void
end_span(char **r, struct a2h *ah) {
  if (ah->f[0] || ah->b[0]) {
    (*r) += sprintf(*r, "</span>");
  }
  if (ah->flags) {
    if (ah->flags & ANSI_INVERT) { *r += sprintf(*r, "</span>"); }
    if (ah->flags & ANSI_HILITE) { *r += sprintf(*r, "</strong>"); }
    if (ah->flags & ANSI_FLASH) { *r += sprintf(*r, "</em>"); }
    if (ah->flags & ANSI_UNDERLINE) { *r += sprintf(*r, "</u>"); }
  }
}

#define _ESC 0x1B
char *
ansi2html(World *w, char *str) {
  int inspan;
  char *r, *s;
  struct a2h *ah = &w->a2h;
  int code;
  int donbsp;
  // With BUFFER_LEN of 8192, this takes 10kb, but it's called for every
  // line and the other solution is to malloc 10*stren(), which isn't wise.
  // We'll probably almost never fill this buffer, but who cares.
  static char ret[BUFFER_LEN * 10];
  // I think this should be enough for every case. If not, shoot me.
  // It's short lived anyway, so who cares?
  r = ret;

  inspan = start_span(&r, ah);
  donbsp = 1;
  while (*str) {
    while (*str && *str != _ESC) {
      switch (*str) {
      case '\t':
        r += sprintf(r, "&nbsp; ");
        donbsp = 1;
        break;
      case ' ':
        if (donbsp) {
          r += sprintf(r, "&nbsp;");
        } else {
          *(r++) = ' ';
        }
        donbsp = !donbsp;
        break;
      case '<':
        r += sprintf(r, "&lt;");
        break;
      case '>':
        r += sprintf(r, "&gt;");
        break;
      case '&':
        r += sprintf(r, "&amp;");
        break;
      case '\n':
        // We should never get \\r or \\n, but just in case . . .
        r += sprintf(r, "\\r");
        break;
      case '\r':
        // We should never get \\r or \\n, but just in case . . .
        r += sprintf(r, "\\r");
        break;
      case '"':
      case '\'':
      case '\\':
        r += sprintf(r, "\\%c", (*str) & 0xFF);
        break;
      default:
        /*
        if (isprint(*str)) {
          *(r++) = *str;
          donbsp = 0;
        } else {
          r += sprintf(r, "\\x%.2x", (*str) & 0xFF);
        }
        */
        *(r++) = *str;
      }
      str++;
    }
    if (inspan) {
      if (r > ret && *(r-1) == ' ') {
        r -= 1;
        r += sprintf(r, "&nbsp;");
      }
      end_span(&r, ah); donbsp = 1;
    }
    inspan = 0;
    if (*str == _ESC) {
      while (*str == _ESC && *(str+1) == '[') {
        str ++;
        do {
          str++;
          s = str;
          while (*str && isdigit(*str)) str++;
          code = atoi(s);
          switch (code) {
          case 30: strcpy(ah->f,"fg_x"); break;
          case 31: strcpy(ah->f,"fg_r"); break;
          case 32: strcpy(ah->f,"fg_g"); break;
          case 33: strcpy(ah->f,"fg_y"); break;
          case 34: strcpy(ah->f,"fg_b"); break;
          case 35: strcpy(ah->f,"fg_m"); break;
          case 36: strcpy(ah->f,"fg_c"); break;
          case 37: strcpy(ah->f,"fg_w"); break;
          case 40: strcpy(ah->b,"bg_x"); break;
          case 41: strcpy(ah->b,"bg_r"); break;
          case 42: strcpy(ah->b,"bg_g"); break;
          case 43: strcpy(ah->b,"bg_y"); break;
          case 44: strcpy(ah->b,"bg_b"); break;
          case 45: strcpy(ah->b,"bg_m"); break;
          case 46: strcpy(ah->b,"bg_c"); break;
          case 47: strcpy(ah->b,"bg_w"); break;
          case 1: ah->flags |= ANSI_HILITE; break;
          case 3: ah->flags |= ANSI_FLASH; break;
          case 4: ah->flags |= ANSI_UNDERLINE; break;
          case 7: ah->flags |= ANSI_INVERT; break;
          case 5: ah->flags |= ANSI_FLASH; break;
          case 6: ah->flags |= ANSI_FLASH; break;
          case 0:
            // Unset everything.
            ah->f[0] = '\0';
            ah->b[0] = '\0';
            ah->flags = 0;
            break;
          case 38: 
            // 256 color foreground
            if ((*str == ';') && (*(str+1) == '5') && *(str+2) == ';') {
              str += 3;
              code = atoi(str);
              while (*str && isdigit(*str)) str++;
              snprintf(ah->f, ANSI_SIZE, "fg_%d", code);
            }
            break;
          case 48: 
            // 256 color background
            if ((*str == ';') && (*(str+1) == '5') && *(str+2) == ';') {
              str += 3;
              code = atoi(str);
              while (*str && isdigit(*str)) str++;
              snprintf(ah->b, ANSI_SIZE, "bg_%d", code);
            }
            break;
          }
        } while (*str == ';');
        if (*str == 'm') str++;
      }
    }
    if (*str) {
      if (r > ret && *(r-1) == ' ') {
        r -= 1;
        r += sprintf(r, "&nbsp;");
      }
      inspan = start_span(&r, ah);
      donbsp = 1;
    }
  }
  *(r) = '\0';
  return strdup(ret);
}

char *
convert_to_json(World *w, char *str) {
  char *post_convert;
  char *post_ansi2html;

  if (strcasecmp(w->charset, "UTF-8")) {
    post_convert = convert_charset(w->charset, "UTF-8", str);
    if (post_convert) {
      post_ansi2html = ansi2html(w, post_convert);
      free(post_convert);
    } else {
      post_ansi2html = ansi2html(w, str);
    }
  } else {
    post_ansi2html = ansi2html(w, str);
  }
  return post_ansi2html;
}

void
send_charsets(World *w) {
  char *charsets = ";UTF-8;ISO-8859-1;ISO-8859-2;US-ASCII;CP437";
  write_raw(w->fd, IAC SB CHARSET CHARSET_REQUEST, 4);
  write_raw(w->fd, charsets, sizeof(charsets));
  write_raw(w->fd, IAC SE, 2);
}

void
send_naws(World *w, int width, int height) {
  char bs[4];
  write_raw(w->fd, IAC SB NAWS, 3);
  bs[0] = width >> 8 & 0xFF;
  bs[1] = width & 0xFF;
  bs[2] = height >> 8 & 0xFF;
  bs[3] = height & 0xFF;
  write_escaped(w->fd, bs, 4);
  write_raw(w->fd, IAC SE, 2);
}

void
send_ttype(World *w, char *what) {
  write_raw(w->fd, "  ", 1);
  write_raw(w->fd, IAC SB TTYPE IS, 4);
  write_escaped(w->fd, what, strlen(what));
  write_raw(w->fd, IAC SE, 2);
}

// When we get our first IAC, we send this.
void
try_iac(World *w) {
  write_raw(w->fd, IAC WILL CHARSET, 3);
  write_raw(w->fd, " ", 1);
}

#define readb(b) b = 0; if (recv_raw(w->fd, &b, 1, 0) < 1) return -1;
int
iac_do(World *w, int b) {
  switch (b) {
  case _LINEMODE:
    write_raw(w->fd, IAC WONT LINEMODE, 3);
    break;
  case _TTYPE:
    write_raw(w->fd, IAC WILL TTYPE, 3);
    break;
  case _NAWS:
    write_raw(w->fd, IAC WILL NAWS, 3);
    send_naws(w, 92, 19);
    break;
  case _CHARSET:
    send_charsets(w);
    break;
  }
  return 0;
}

int
iac_will(World *w, int b) {
  switch (b) {
  case _EOR:
    write_raw(w->fd, IAC DO EOR, 3);
    break;
  case _MSSP:
    write_raw(w->fd, IAC DO MSSP, 3);
    break;
  case _NAWS:
    write_raw(w->fd, IAC DO NAWS, 3);
    send_naws(w, 92, 19);
    break;
  case _CHARSET:
    write_raw(w->fd, IAC DO CHARSET, 3);
    break;
  }
  return 0;
}

int
iac_sb(World *w, int b) {
  char charset[20];
  int i;
  switch (b) {
  case _TTYPE:
    readb(b); // SEND
    readb(b); // IAC
    readb(b); // SE
    send_ttype(w, "Banana");
    break;
  case _MSSP:
    while (1) {
      readb(b);
      if (b == _IAC) {
        readb(b);
        if (b != _IAC) {
          return 0;
        }
      }
    }
    break;
  case _CHARSET:
    readb(b);
    if (b != _CHARSET_ACCEPTED) {
      readb(b); // IAC
      readb(b); // SE
    } else {
      for (i = 0; i < 20; i++) {
        readb(b);
        if (b == _IAC) break;
        charset[i] = b;
      }
      if (i == 20) {
        while (b != _IAC) {
          readb(b);
        }
      }
      readb(b); // SE
      slog("%s.%s: Charset '%s' accepted.", w->user->name, w->name, charset);
      snprintf(w->charset, 20, "%s", charset);
    }
  }
  return 0;
}

int
handle_iac(World *w, int b) {
  // W got an IAC command. If we haven't dealt with IAC before, then
  // send our own connection string.
  if (w->tried_iac == 0) {
    w->tried_iac = 1;
    try_iac(w);
  }
  switch (b) {
  case _DO:
    readb(b);
    return iac_do(w,b);
  case _WILL:
    readb(b);
    return iac_will(w,b);
  case _EOR:
  case _GA:
    return 1;
  case _SB:
    readb(b);
    return iac_sb(w,b);
  case _DONT:
  case _WONT:
    readb(b);
    // Okay, we won't?
  }
  return 0;
}
#undef readb

#define MAX_LINES 200
// raw_read: Returns 1 if the socket is closed, otherwise 0.
int
raw_read(World *w) {
  int   ret;
  char *buff = w->buff;
  int  *lbp = &w->lbp;
  char *lines[MAX_LINES];
  char *prompt = NULL;
  int   nlines = 0;
  int   i;
  int   b = 0;

#define readb(b) b = 0; if ((ret = recv_raw(w->fd, &b, 1, 0)) < 1) goto exit_sequence
  while (1) {
    readb(b);
    switch (b) {
    case '\r':
      break;
    case '\n':
      buff[*lbp] = '\0';
      lines[nlines++] = convert_to_json(w,buff);
      *lbp = 0;
      if (nlines >= MAX_LINES) {
        // Don't read no more, just wait until next one.
        goto exit_sequence;
      }
      break;
    case _IAC:
      readb(b);
      if (b != _IAC) {
        ret = handle_iac(w,b);
        if (ret < 0) {
          goto exit_sequence;
        } else if (ret > 0) {
          buff[*lbp] = '\0';
          prompt = convert_to_json(w,buff);
          goto exit_sequence;
        }
        break;
      }
    default:
      // TODO: If lbp starts approaching being  too big,
      // push the line as it is. Maybe at buffer_len - 30, start looking
      // for spaces?
      buff[(*lbp)++] = b;
    }
  }
#undef readb

exit_sequence:
  if (nlines > 0) {
    addLines(w->user, w, lines, nlines);
    for (i = 0; i < nlines; i++) {
      free(lines[i]);
    }
  }
  if (prompt) {
    queueEvent(w->user, w, 1, EVENT_WORLD_PROMPT,
               "world:'%s',text:'%s'", w->name, prompt);
    free(prompt);
  }
  return ret == 0;
}

void
net_read(World *w) {
  struct epoll_event epvt;
  net_lock();
  noisy_lock(&w->user->mutex, w->user->name);

  // Read data. Just throw it away for now.
  if (raw_read(w)) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, w->fd, &epvt);
    world_dc_event(w->user, w);
    w->fd = -1;
    w->netstatus = WORLD_DISCONNECTED;
  }
#ifdef DEBUG_NET
  printf("\n");
  writing = -1;
#endif
  noisy_unlock(&w->user->mutex, w->user->name);
  net_unlock();
}

void
net_poll() {
  struct epoll_event events[MAX_EVENTS];
  time_t now = time(NULL);
  time_t next = time(NULL) + CLEANUP_TIMEOUT;
  int numevents;
  struct epoll_event epvt;
  World *w;
  int i;

  while (1) {
    numevents = epoll_wait(epfd, events, MAX_EVENTS, (next - now) * 1000);
    net_lock();
    for (i = 0; i < numevents; i++) {
      w = (World *) events[i].data.ptr;
      // Make sure we haven't been removed for bad behaviour.
      // TODO: A better algorithm?
      if (w->netstatus != WORLD_CONNECTED) continue;
      // If we got here, we have valid data to process. Or rather,
      // we have a valid socket to work on.
      if (events[i].events & EPOLLIN) {
        net_read(w);
      }
      if (events[i].events & EPOLLERR) {
        close(w->fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, w->fd, &epvt);
        world_dc_event(w->user, w);
        w->fd = -1;
        w->netstatus = WORLD_DISCONNECTED;
      }
    }
    now = time(NULL);
    if (now >= next) {
      sessions_expire();
      users_cleanup();
      next = now + CLEANUP_TIMEOUT;
    }
    net_unlock();
  }
}
