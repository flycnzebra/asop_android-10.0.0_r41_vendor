 
#include "head.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>




#include <sys/ioctl.h>
#include <net/if.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/udp.h>
#endif


#include "netlib.h"



/* Get interface address */
unsigned long getifaddr(char * ifname) 
{
     struct sockaddr_in addr;
     struct ifreq ifr;
     int s;

     if( (s = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
        return -1;

     strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
     ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';

     if( ioctl(s, SIOCGIFADDR, &ifr) < 0 ){
        close(s);
        return -1;
     }
     close(s);

     addr = *((struct sockaddr_in *) &ifr.ifr_addr);

     return addr.sin_addr.s_addr;
}


int establishUdpSession(struct MPC_HOST *host,char * physicalIfname,bool isCommandMessage) 
{
     struct sockaddr_in saddr; 
     short port;
     int s,opt;
    
     if( (s=socket(PF_INET, SOCK_DGRAM,0))== -1 ){
        MPCLOG(LLV_ERROR,"Can't create socket  failed. %s(%d) \n",strerror(errno), errno);
        return -1;
     }
    
    opt=1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
  
    struct ifreq struIR;
    memset(&struIR, 0, sizeof(struIR));
    strncpy(struIR.ifr_ifrn.ifrn_name, physicalIfname,strlen(physicalIfname));
    if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE,(char *)&struIR, sizeof(struIR))< 0) {
      
        MPCLOG(LLV_ERROR,"SO_BINDTODEVICE  physicalIfname = %s failed. %s(%d)\n",physicalIfname,strerror(errno), errno);
        
        close(s);  
        return -1;
    }
    struct sockaddr_in addrServer;
    addrServer.sin_family=AF_INET;  
    addrServer.sin_addr.s_addr=inet_addr(mpc_op.svr_addr);  
    addrServer.sin_port=htons(isCommandMessage?mpc_op.svr_command_port:mpc_op.svr_data_port); 
   
    if( connect(s,(struct sockaddr *)&addrServer,sizeof(addrServer)) ){
        MPCLOG(LLV_ERROR,"Can't connect socket on physicalIfname : %s,ERROR:%s(%d) \n",physicalIfname,strerror(errno), errno);
        return -1;
    }

    return s;

}




int udp_write(int fd, char *buf, int len)
{
   
     register int wlen;
   
     if(fd<=0){
        MPCLOG(LLV_ERROR,"udp_write error For fd is Unknow\n");
        return -1;
     }

     int i=0;
     while( i<1 ){        
        if( (wlen = write(fd, buf, len)) < 0 ){  
            //MPCLOG(LLV_ERROR,"udp_write error on fd  = %d ,buf = %p,len = %d,errno = %d  ,wlen = %d, errNum = %d\n",fd,buf,len,errno,wlen,i);

            if(errno==ENODEV || (errno==EINVAL)){//:19 No such device ,22: invalid argument
                //close(fd);
                return -1;
            }

            if(errno==EFAULT){
                return 0;

            }else if( errno == EAGAIN ||
                errno == EINTR){
                i++;
                usleep(5*1000);
                continue;
            }else if( errno == ENOBUFS ||
                 errno==ENOMEM){
                i++;
                usleep(5*1000);
                continue;
            }else
                return 0;
        }                   
        return wlen;
     }
     return 0;
}

int udp_read(int fd, char *buf,int len)
{    
     
     register int rlen; 
    
     int i=0;
     while( i<5 ){
        
        if( (rlen = read(fd, buf, len)) < 0 )
        {
            MPCLOG(LLV_ERROR,"udp_read error on fd  = %d ,errno = %d , buf = %p,errNum =%d\n",fd,errno,buf,i);

            if( errno == EAGAIN || errno == EINTR ){
                 i++;
                usleep(1);
                continue;
            }else{                
                return rlen;
            }
        }       
       
        return rlen;
     }
     return rlen;
}  


void addMpRouteAndIptables(){

    char _command[100];
    char * routes[2];
    //routes[0] = "iptables -t nat -A OUTPUT -p udp --dport 53 -j DNAT --to-destination %s:53";
    routes[0] = "ip rule add from 0.0.0.0/0.0.0.0 table %s pref 18001";
    routes[1] = "ip route add table %s default via %s";
    
    int i = 0;
    for(i=0;i<2;i++){
        memset(_command, 0, sizeof(_command));
        //if(i==0)
        //    sprintf(_command,routes[i], mpc_op.dns_addr);
        if(i==0)
            sprintf(_command,routes[i], mpc_op.table_name);
        else{            
                   
            sprintf(_command,routes[i],mpc_op.table_name, mpc_op.localIp);
            
        }
        //MPCLOG(LLV_INFO,"%s System %s\n", __func__,_command);
        system(_command);
    }
   
}


void clearMpRouteAndIptables(){
    char _command[100];
    //char * _iptables = "iptables -t nat -D OUTPUT -p udp --dport 53 -j DNAT --to-destination %s:53";
    char * _route = "ip rule del from 0.0.0.0/0.0.0.0 table %s pref 18001";
    char * _route2 = "ip route flush table %s";

    //memset(_command, 0, sizeof(_command));
    //sprintf(_command,_iptables, mpc_op.dns_addr);
    //MPCLOG(LLV_INFO,"%s System %s\n", __func__,_command);
    //system(_command);
    memset(_command, 0, sizeof(_command));
    sprintf(_command,_route, mpc_op.table_name);
    //MPCLOG(LLV_INFO,"%s System %s\n", __func__,_command);
    system(_command);
    memset(_command, 0, sizeof(_command));
    sprintf(_command,_route2, mpc_op.table_name);
    //MPCLOG(LLV_INFO,"%s System %s\n", __func__,_command);
    system(_command);
    
}

