/** net.h
 *
 * Network library for Banana.
 */

#ifndef _MY_NET_H_
#define _MY_NET_H_

#define MAX_EVENTS 20
#define WAIT_TIMEOUT 1000

struct user;
struct world;

int net_init();
void net_poll();

inline void net_lock();
inline void net_unlock();
void net_connect(struct world *world, char *host, char *port);
void net_disconnect(struct world *world);
void net_send(World *world, char *text);

// WARNING: remove_markup DESTRUCTIVELY modifies the string.
char *remove_markup(char *str);

#endif
