
#ifndef _IFADDRS_H_
#define _IFADDRS_H_
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char    *ifa_name;
    unsigned int    ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_dstaddr;
    void    *ifa_data;
};

#ifndef ifa_broadaddr
#define ifa_broadaddr   ifa_dstaddr 
#endif
#include <sys/cdefs.h>
__BEGIN_DECLS
extern int getifaddrs(struct ifaddrs **ifap);
extern void freeifaddrs(struct ifaddrs *ifa);
__END_DECLS
#endif
