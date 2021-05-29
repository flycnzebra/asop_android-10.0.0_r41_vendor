
#include "head.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>

#include <netinet/in.h>  

#include <linux/if_tun.h>

#define OTUNSETIFF     (('T'<< 8) | 202) 

int init_tun_network(struct MPC_HOST *host){

    int fd=-1;
    int interface_already_open = 0;
    char dev[MPC_DEV_LEN]="";
    if ( (host->loc_fd > 0) )
        interface_already_open = 1;
    
    if( host->dev ){
        strncpy(dev, host->dev, MPC_DEV_LEN);
        dev[MPC_DEV_LEN-1]='\0';
    }

    if( ! interface_already_open ){
        
        if( (fd=tun_open(dev)) < 0 ){
            MPCLOG(LLV_ERROR,"Can't allocate tun device %s. %s(%d)\n", 
                dev, strerror(errno), errno);
            return -1;
        }
          
        host->loc_fd = fd;
    }

    return 0;
}

static int tun_open_common(char *dev, int istun)
{
    struct ifreq ifr;
    int fd;
    int tun_mtu = 1428;

    if ((fd = open("/dev/tun", O_RDWR)) < 0)
       goto failed;

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = (istun ? IFF_TUN : IFF_TAP) | IFF_NO_PI;
    if (*dev)
       strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
       if (errno == EBADFD) {      
      if (ioctl(fd, OTUNSETIFF, &ifr) < 0) 
         goto failed;
       } else
          goto failed;
    } 

   if(set_ip(dev,mpc_op.localIp,"255.255.255.252",tun_mtu)!=1)   
  
        goto failed;
   
    strcpy(dev, ifr.ifr_name);
    return fd;

failed:
    MPCLOG(LLV_ERROR,"      %s .................... failed\n",__func__);
    close(fd);
    return -1;
}



int set_ip( char *dev, char *ipaddr, char *netmask ,int mtu)
{
    struct ifreq ifr;
    int err;
    
    int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
    
    
    if ( *dev ) 
    {
        strncpy( ifr.ifr_name, dev, IFNAMSIZ );
    }
    ifr.ifr_addr.sa_family = AF_INET;
    
    if( ( err = inet_pton( AF_INET, ipaddr, ifr.ifr_addr.sa_data + 2 ) ) != 1 )
    {
        close( fd );
        return err;
    }
    
    if( ( err = ioctl( fd, SIOCSIFADDR, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }    
    
    if( ( err = inet_pton( AF_INET, netmask, ifr.ifr_addr.sa_data + 2 ) ) != 1 )
    {
        close( fd );
        return err;
    }
    if( ( err = ioctl( fd, SIOCSIFNETMASK, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }
    
    ifr.ifr_ifru.ifru_mtu = mtu;

    if( ( err = ioctl( fd, SIOCSIFMTU, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }
    ifr.ifr_ifru.ifru_ivalue = 1000;
    if( ( err = ioctl( fd, SIOCSIFTXQLEN, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }	
			
    if( ( err = ioctl( fd, SIOCGIFFLAGS, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }
    ifr.ifr_flags |= ( IFF_UP | IFF_RUNNING );
    ifr.ifr_flags &=~IFF_NOARP;

    
    if( ( err = ioctl( fd, SIOCSIFFLAGS, &ifr ) ) < 0 )
    {
        close( fd );
        return err;
    }
    
    close( fd );
    
    return 1;
}

int tun_open(char *dev) { 
    return tun_open_common(dev, 1); 
}
int tun_close(int fd, char *dev) { 
    if(fd>0)
        return close(fd); 
    else 
        return -1;
}
int tun_write(int fd, char *buf, int len) { return write(fd, buf, len); }

int tun_read(int fd, char *buf, int len) { return read(fd, buf, len); }

