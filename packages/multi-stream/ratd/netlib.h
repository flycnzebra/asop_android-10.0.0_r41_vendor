

#ifndef _MPC_NETLIB_H
#define _MPC_NETLIB_H

//#include "config.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

unsigned long getifaddr(char * ifname);
void clearMpRouteAndIptables();
void addMpRouteAndIptables();
int udp_read(int fd, char *buf,int len);

int udp_write(int fd, char *buf, int len);

int establishUdpSession(struct MPC_HOST *host,char * physicalIfname,bool isCommandMessage) ;
#endif /* _VTUN_NETDEV_H */
