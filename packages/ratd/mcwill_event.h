#ifndef	_MCWILL_EVENT_H_
#define	_MCWILL_EVENT_H_

#include <stdbool.h>

#define MAX_FD_EVENTS 15
#define DEBUG 1


typedef void (*mcwill_event_cb)(int fd, short events, void *userdata,short flags);
struct mcwill_event {
	struct mcwill_event *next;
	struct mcwill_event *prev;

	int fd;
	int index;bool persist;
	struct timeval timeout;
	char evname[40];	
	mcwill_event_cb func;
    int netType;
	void *param;
};

void mcwill_event_init();

void mcwill_event_set(struct mcwill_event * ev, int fd, bool persist,
		mcwill_event_cb func, void * param, char evname[],short netType);

void mcwill_event_add(struct mcwill_event * ev);

void mcwill_timer_add(struct mcwill_event * ev, struct timeval * tv);

void mcwill_event_del(struct mcwill_event * ev,int * pFd);
void printPendingAndWatchTableListWithLock();

void mcwill_event_loop();
void getNow(struct timeval * tv);
#endif

