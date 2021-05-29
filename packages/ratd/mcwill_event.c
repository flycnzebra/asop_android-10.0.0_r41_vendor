#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "mcwill_event.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <pthread.h>
#include "plog.h"

static pthread_mutex_t listMutex;
#define MUTEX_ACQUIRE() pthread_mutex_lock(&listMutex)
#define MUTEX_RELEASE() pthread_mutex_unlock(&listMutex)
#define MUTEX_INIT() pthread_mutex_init(&listMutex, NULL)
#define MUTEX_DESTROY() pthread_mutex_destroy(&listMutex)

#ifndef timeradd
#define timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#endif

#ifndef timercmp
#define timercmp(a, b, op)               \
        ((a)->tv_sec == (b)->tv_sec      \
        ? (a)->tv_usec op (b)->tv_usec   \
        : (a)->tv_sec op (b)->tv_sec)
#endif

#ifndef timersub
#define timersub(a, b, res)                           \
    do {                                              \
        (res)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
        (res)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((res)->tv_usec < 0) {                     \
            (res)->tv_usec += 1000000;                \
            (res)->tv_sec -= 1;                       \
        }                                             \
    } while(0);
#endif

fd_set g_readFds;
int g_nfds = 0;


struct mcwill_event * watch_table[MAX_FD_EVENTS];

struct mcwill_event timer_list;
struct mcwill_event pending_list;

static void init_list(struct mcwill_event * list) {
	memset(list, 0, sizeof(struct mcwill_event));
	list->next = list;
	list->prev = list;
	list->fd = -1;
}

static void addToList(struct mcwill_event * ev, struct mcwill_event * list) {
      
    ev->next = list;    
	ev->prev = list->prev;    
	ev->prev->next = ev;
	list->prev = ev;
	
}

static void removeFromList(struct mcwill_event * ev) {

    if(ev!=NULL&&ev->next!=NULL&&ev->prev!=NULL){
	    ev->next->prev = ev->prev;
	    ev->prev->next = ev->next;
	    ev->next = NULL;
	    ev->prev = NULL;
    }
}
static void removeWatchByName(struct mcwill_event *ev,char * name){

}

static void removeWatch(struct mcwill_event * ev, int index) {

	watch_table[index] = NULL;
	ev->index = -1;
	
	FD_CLR(ev->fd, &g_readFds);

	if (ev->fd + 1 == g_nfds) {
		int n = 0;
		int i = 0;
		for (i = 0; i < MAX_FD_EVENTS; i++) {
			struct mcwill_event * rev = watch_table[i];

			if ((rev != NULL) && (rev->fd > n)) {
				n = rev->fd;
			}
		}
		g_nfds = n + 1;
		
	}
	
}


static void processTimeouts()
{
    MUTEX_ACQUIRE();
    struct timeval now;
    struct mcwill_event * tev = timer_list.next;
    struct mcwill_event * next;

    getNow(&now);
   
    while ((tev != &timer_list) && (timercmp(&now, &tev->timeout, >))) {
        
        next = tev->next;
        removeFromList(tev);
        addToList(tev, &pending_list);
        tev = next;
    }
    MUTEX_RELEASE();
    
}



static void processReadReadies(fd_set * rfds, int n) {

	MUTEX_ACQUIRE();
	int i = 0;
	for (i = 0; (i < MAX_FD_EVENTS) && (n > 0); i++) {
		struct mcwill_event * rev = watch_table[i];
		if (rev != NULL && FD_ISSET(rev->fd, rfds)) {
          
			addToList(rev, &pending_list);
			if (rev->persist == false) {
				removeWatch(rev, i);
			}
			n--;
		}
	}

	MUTEX_RELEASE();
	
}

static void firePending() {
	
	struct mcwill_event * ev = pending_list.next;
	while (ev != &pending_list) {
		struct mcwill_event * next = ev->next;
		removeFromList(ev);

        if(ev->func==NULL)
            MPCLOG(LLV_ERROR, "firePending ~~~~ call ev->func,but ev->func is NULL");
        else
		    ev->func(ev->fd, 0, ev->param,ev->netType);
		ev = next;
	}
	
}

void mcwill_event_init() {
	MUTEX_INIT();

	FD_ZERO(&g_readFds);
	init_list(&timer_list);
	init_list(&pending_list);
	memset(watch_table, 0, sizeof(watch_table));
}

void mcwill_event_set(struct mcwill_event * ev, int fd, bool persist,
		mcwill_event_cb func, void * param, char evname[],short netType) {

	memset(ev, 0, sizeof(struct mcwill_event));
	ev->fd = fd;
	ev->index = -1;	
	strncpy(ev->evname, evname, sizeof(ev->evname)>strlen(evname)?strlen(evname):sizeof(ev->evname));
	ev->persist = persist;
	ev->func = func;
    ev->netType = netType;
	ev->param = param;
   
	if(fcntl(fd, F_SETFL, O_NONBLOCK)<0&&fd>0){
        MPCLOG(LLV_ERROR, "mcwill_event_set fd = %d ,name = %s ,netType = %d ~~~~ Error !!!!!\n", fd,evname,netType);
	}
	
}

void mcwill_event_add(struct mcwill_event * ev) {
	
	MUTEX_ACQUIRE();
	int i = 0;
    
	for (i = 0; i < MAX_FD_EVENTS; i++) {
		if (watch_table[i] == NULL) {
			watch_table[i] = ev;
			ev->index = i;
			
			FD_SET(ev->fd, &g_readFds);
			if (ev->fd >= g_nfds)
				g_nfds = ev->fd + 1;
			 
			break;
		}else if(watch_table[i]!=NULL&&watch_table[i]->fd == ev->fd){
         	ev->index = i;
            
            break;
        }
	}
    
	MUTEX_RELEASE();
	
}


void mcwill_event_del(struct mcwill_event * ev,int * pFd) {
	
	MUTEX_ACQUIRE();
   
    //if (ev->index < 0 || ev->index >= MAX_FD_EVENTS) {
	if (ev->index < 0 || ev->index >= MAX_FD_EVENTS|| (ev->fd ==0&&ev->index==0)) {//huajinshuihuajinshui
        MPCLOG(LLV_ERROR, "~~~~~~ mcwill_event_del(Index is invalid) fd = %d,name = %s index = %d    ~~~~~\n", ev->fd,ev->evname,ev->index);

        
	}else{
        MPCLOG(LLV_ERROR, "~~~~~~ mcwill_event_del fd = %d,name = %s index = %d~~~~~\n", ev->fd,ev->evname,ev->index);

	    removeWatch(ev, ev->index);

        //printfWatchTableEv();
	}
     if(*pFd>0){
        close(*pFd);
        *pFd=-1;
    }
	MUTEX_RELEASE();

}

bool mcwill_timer_check_exist(struct mcwill_event * ev){
    MUTEX_ACQUIRE();
    struct mcwill_event * list;

    bool result=false;
    
    list = timer_list.next;
    do{
        if(list==ev){                
                result =  true;
                break;
            }       
        
        list = list->next;
    }while ((list != &timer_list));

    MUTEX_RELEASE();

    return result;
}


static int timeNum =0;
void mcwill_timer_add(struct mcwill_event * ev, struct timeval * tv)
{
    
    MUTEX_ACQUIRE();
    struct mcwill_event * list;
    if (tv != NULL) {
        list = timer_list.next;
        ev->fd = -1; 
        
        struct timeval now;
        getNow(&now);
        timeradd(&now, tv, &ev->timeout);
        
        while (timercmp(&list->timeout, &ev->timeout, < )
                && (list != &timer_list)) {
            list = list->next;
        }
        
        addToList(ev, list);
        
    }
   
    MUTEX_RELEASE();
    
}



static int calcNextTimeout(struct timeval * tv)
{
    struct mcwill_event * tev = timer_list.next;
    struct timeval now;

    getNow(&now);

    if (tev == &timer_list) {
        
        return -1;
    }

    if (timercmp(&tev->timeout, &now, >)) {
        timersub(&tev->timeout, &now, tv);
    } else {
        
        tv->tv_sec = tv->tv_usec = 0;
    }
    return 0;
}

static unsigned long long j=0LLU;
    
#if DEBUG
static void printReadies(fd_set * rfds,bool showReady,int readyFd)
{
    
}
#else
#define printReadies(rfds,showReady) do {} while(0)
#endif

void mcwill_event_loop() {
	int n;
    fd_set rfds;
    struct timeval tv;
    struct timeval * ptv;
    int numForEbadf = 0;
    
	for (;;) {
        MUTEX_ACQUIRE();
		memcpy(&rfds, &g_readFds, sizeof(fd_set));
        MUTEX_RELEASE();
		if (-1 == calcNextTimeout(&tv)) {           
           ptv = NULL;
        } else {
           
            ptv = &tv;
        }
        n = select(g_nfds, &rfds, NULL, NULL, ptv);

		if (n < 0) {
            // EINTR 4 /* Interrupted system call */   EBADF 9 /* Bad file number */ EAGAIN 11 /* Try again */
            MPCLOG(LLV_ERROR,"mcwill_event_loop errno:%d,%s", errno,strerror(errno));

            if(errno == EBADF)
            {
                numForEbadf++;
                if(numForEbadf<3)continue;else return;
            }else{
                numForEbadf = 0;
                if (errno == EINTR||errno == EAGAIN)continue;else return;
                
            }
			
                        
			return;
		}        
        processTimeouts();       
        processReadReadies(&rfds, n);        
        firePending();
		
	}
}

void getNow(struct timeval * tv)
{
#ifdef HAVE_POSIX_CLOCKS
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec/1000;
#else
    gettimeofday(tv, NULL);
#endif
}
