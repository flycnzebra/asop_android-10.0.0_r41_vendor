

#include "queue.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/mman.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


#include "head.h"

#include <sys/wait.h>
#include <sys/un.h>
#include <cutils/sockets.h>


#include<pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <linux/kernel.h>

#include<sys/types.h>  
#include<sys/stat.h>  
#include<stdio.h>  
#include<fcntl.h>  
#include<signal.h>  
#include<unistd.h>


#include<stdio.h>
#include<time.h>
#include<sys/time.h>


#include <linux/udp.h>
#include <linux/tcp.h>


//#include <linux/module.h>  // For Android 4.4
#include <linux/string.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/socket.h>



extern const stMPDFunc *stMpdFuncs;
extern struct MPC_HOST mpc_host;
extern bool g_netStatus[3];


int MPC_OnPostPreMpDataToTransport(const char *buffer, size_t datalen,size_t offsize){

    char * buf = buffer+offsize;
    struct iphdr *newIphm = (struct iphdr *)buf;
    struct in_addr addr1;
    int len = datalen-offsize;
   
    

    int ret =  tun_write(mpc_host.loc_fd,buf,len);      
    if(buffer){
        free(buffer);buffer = NULL;
        
    }
    
    return ret;
    
    
}




int MPC_OnNotifyException(int netType,int exceptionCode){
    
    MPCLOG(LLV_ERROR, " MPC_OnNotifyException(netType = %d,exceptionCode = %d) \n",netType,exceptionCode);

    return echoAppException(exceptionCode,netType);
    
}

int mp_data_send(char *buf,int len){

    int res = 0;
    if(stMpdFuncs!=NULL){
        MPCLOG(LLV_INFO, "to call stMpdFuncs->onSendNewTransData................ \n");

        res = stMpdFuncs->onSendNewTransData(buf,len);        
     
        return res;
    }else{
        MPCLOG(LLV_ERROR, "stMpdFuncs is NULL\n ");
        return -1;
    }
    
}


int MPC_OnSendMpDataComplete(int netType, char * buffer, int len,int release){

  return MPC_putMsgToSendQueue(netType/2,buffer,len,release);
        
}


int mp_data_recv_to_transport(int fd,char * buffer,int len){

   
    int result = stMpdFuncs->onParseMPData(buffer,len);
   
    return result;
    
    
}

extern QUEUE Q_MP_DISPATCH[3];

void Free_ResendBuf(char *buffer,int subFlowFlag){
   MPCLOG(LLV_ERROR," %s buffer =%p,subFlowFlag = %d\n", __func__,buffer,subFlowFlag);
   if(subFlowFlag<3) 
       DequeFreeReTransBuffer(&Q_MP_DISPATCH[subFlowFlag],buffer);
  
}

int MPC_putMsgToSendQueue(int fdIndex,char * buffer,int len,int release){

       
    struct ElemType elem ;
    memset(&elem,0x0,sizeof(struct ElemType));
    elem.bufferAddr = buffer;
    elem.len        = len;
    elem.release    = release;
    elem.fdIndex    = fdIndex;

    if(true ==  Enqueue(fdIndex,&Q_MP_DISPATCH[fdIndex],&elem))
        return 0;
    else{
        if(elem.release==true){            
            if(false == ResetElemFlagInQueue(&Q_MP_DISPATCH[fdIndex],elem.bufferAddr,elem.release))
                free(elem.bufferAddr);
           
        }
         return -1;
    }
    
    

}


