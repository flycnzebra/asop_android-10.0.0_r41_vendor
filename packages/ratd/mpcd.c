#include "head.h"
#include "mcwill_event.h"
#include "record_stream.h"
#include <linux/kernel.h>

#include<sys/types.h>  
#include<sys/stat.h>  
#include<stdio.h>  
#include<fcntl.h>  
#include<signal.h>  
#include<unistd.h>


#define SOCKET_NAME "socket_ratd"

typedef void *(*TASKFUNC)(void *);
int g_fdListen = -1;
int g_fdNetlink = -1;
int g_fdConnected = -1;
int g_log_enable = false;

extern int g_nfds;
//extern FILE * fp_log;
extern fd_set g_readFds;
static int s_started = 0;
static pthread_t s_tid_reader;
static pthread_t s_tid_dispatch;
static pthread_t s_tid_sack;
static pthread_t s_tid_keepAlive;
static pthread_t s_mp_dispatch[3];
const stMPDFunc *stMpdFuncs=NULL;
const stMPDFunc *(*mpd_Init)(unsigned int ,const struct MPC_Env *);
unsigned long GetRTTC(int netType);

struct MPC_OPTS mpc_op;
struct MPC_HOST mpc_host;
static int s_fdWakeupRead;
static int s_fdWakeupWrite;
static bool s_watchDog_reset = false;
static int s_mpstatus    = MP_DISACTIVE;

static struct mcwill_event * p_ack_event = NULL;


static struct mcwill_event s_tun_event;
static struct mcwill_event s_listen_event;
static struct mcwill_event s_commands_event;
static struct mcwill_event s_wakeupfd_event;

static struct mcwill_event s_netlink_event;
static struct mcwill_event s_wake_timeout_event;

static pthread_mutex_t s_writeMutex         = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_startupMutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_threadMutex_eth    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_threadMutex_rndis  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_startupCond         = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_dispatchMutex      = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_dispatchCond        = PTHREAD_COND_INITIALIZER;

QUEUE Q_MP_DISPATCH[3];
bool g_netStatus[3];

int64_t g_keepAliveinfo =0l;//ms

extern int64_t myUptimeMillis();


void do_plog_empty(int level, char *format, ...){};

static struct MPC_Env s_mpcEnv = {
    MPC_OnPostPreMpDataToTransport
    ,MPC_OnSendMpDataComplete
    ,MPC_OnNotifyException 
    //,do_plog_empty
    ,do_plog
    ,Free_ResendBuf
    ,GetRTTC
};

int mcwill_socket_init();
int removeAllSubFlowsEvent();
int getFdIndexFromQ(PQUEUE Q);
void initDefaultParams();
void mcwill_startEventLoop(void);
void mcwill_startTunDataReadLoop();
void finishsubflowDispathTask(__u8 numberType);
void switchMPToActivite();

unsigned long getTimeStamp();

static void triggerEvLoop();
static void setWatchDog();
static void enableAckTimer();

static void *eventLoop(void *param);
static void echoAppAllSubStatistics();
static void *tunDataReadLoop(void *param);
static void *task_mpMsgQueueDispatch(PQUEUE Q);
static void mcwillEventAddWakeup(struct mcwill_event *ev);

static void userTimerAckCallback(int fd, short flags, void *param,short flags2);
static void processWakeupCallback(int fd, short flags, void *param,short flags2);

static void systemTimerWatchDogCallback(int fd, short flags, void *param,short flags2);
static void *sack_task(void *param);

extern void parseBinaryNetlinkMessage(struct nlmsghdr *nh);
extern void parseNetlinkAddrMsg(struct nlmsghdr *nlh, int new);
extern void netLinkChangedCallback(int fd, short flags, void *param,short flags2);
extern void listenMcwillSocketCallback(int fd, short flags, void *param,short flags2);

int  sendCommandMsgOnAllSubflows(char *buffer ,int len);
void tunDataRecvCallback(int fd, short flags, void *param,short flags2);
void commandDataRecvCallback(int fd, short flags, void *param,short netType);
void processAppCommandsCallback(int fd, short flags, void *param,short flags2);
int  enableTimerTask(int interval,struct mcwill_event * ev,mcwill_event_cb func,char evname[]);
int  enableTimerTask_second(int interval,struct mcwill_event * ev,mcwill_event_cb func,char evname[]);
void toReleaseMpResource();
static void *skeepAlive_task(void *param);

void removeSubFlowsEvent(__u8 numberType);
void removeSpecifiedSubFlow(int fdIndex);
void mcwill_startKeepAliveCheckTask();
void updateCurrentSubFlowsNum();
void notifyMpdNetStatus(int netType,int netStatus);

void task_queue(int fdIndex,TASKFUNC fTaskFun, int cTaskPriority) {
    
	pthread_attr_t attr;
	struct sched_param priority;
	priority.sched_priority = cTaskPriority;

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedparam(&attr, (struct sched_param*) &priority);    
	if (0 == pthread_create(&s_mp_dispatch[fdIndex], &attr, fTaskFun, &Q_MP_DISPATCH[fdIndex])) {
		
	}
   
}


int main(int argc, char **argv) {

    void *dlHandle;
    const char * mpdLibPath = MPD_LIB;
  
    initDefaultParams();   
    clearMpRouteAndIptables();
    
	mcwill_startEventLoop();
    int i =0;
    for( i = 0;i<3;i++){
        g_netStatus[i]=true;
    }
	
	if (mcwill_socket_init() < 0) {
 
		MPCLOG(LLV_ERROR, "socket_ratd Init fail...\n");
		exit(-1);

	} else {
        
        dlHandle = dlopen(mpdLibPath, RTLD_NOW);
        if (dlHandle == NULL) {
            MPCLOG(LLV_ERROR,"dlopen failed: %s\n", dlerror()); 
            close(g_fdListen);
            exit(EXIT_FAILURE);
        }

        mpd_Init = (const stMPDFunc *(*)(const struct MPC_Env *))dlsym(dlHandle, "MPD_Init");
        if (mpd_Init == NULL) {
            MPCLOG(LLV_ERROR,"MPD_Init not defined or exported in %s\n", mpdLibPath); 
            close(g_fdListen);
            exit(EXIT_FAILURE);
        }


        MPCLOG(LLV_ERROR, "listening on LocalSocket\n");        
		mcwill_event_set(&s_listen_event, g_fdListen, false,
				listenMcwillSocketCallback, NULL, "fd_listen",0);

		mcwillEventAddWakeup(&s_listen_event);

        if(netlink_socket_init()<0){
            MPCLOG(LLV_ERROR, "netlink_socket_init failed \n");
        }else{
            mcwill_event_set(&s_netlink_event, g_fdNetlink, true,
    				netLinkChangedCallback, NULL, "fd_net",0);
    		mcwillEventAddWakeup(&s_netlink_event);
        }        
       
		while(1){  			
			sleep(0x00ffff); 
		}  	
	}
    
	return 0;

}



void mcwill_startEventLoop(void) {
	int ret;
	pthread_attr_t attr;
	
	s_started = 0;
	pthread_mutex_lock(&s_startupMutex);


	pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
	struct sched_param param;
	param.sched_priority = 10;
	pthread_attr_setschedparam(&attr,&param);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&s_tid_dispatch, &attr, eventLoop, NULL);

	while (s_started == 0) {
		pthread_cond_wait(&s_startupCond, &s_startupMutex);
	}
    
	pthread_mutex_unlock(&s_startupMutex);

	if (ret < 0) {
		
		return;
	}
}

static void *
eventLoop(void *param) {
	int ret;
	int filedes[2];
    int policy=-1, priority=-1;

	mcwill_event_init();

    pthread_getschedparam(pthread_self(), &policy, (struct sched_param *)&priority);
    
    MPCLOG(LLV_ERROR,"eventLoop   pthread_self() = %lu,policy=%d, priority=%d\n",pthread_self(), policy, priority);  
    

	pthread_mutex_lock(&s_startupMutex);

	s_started = 1;
	pthread_cond_broadcast(&s_startupCond);
	pthread_mutex_unlock(&s_startupMutex);

	ret = pipe(filedes);

	if (ret < 0) {
		
		return NULL;
	}

	s_fdWakeupRead = filedes[0];
	s_fdWakeupWrite = filedes[1];

	fcntl(s_fdWakeupRead, F_SETFL, O_NONBLOCK);

	mcwill_event_set(&s_wakeupfd_event, s_fdWakeupRead, true,
			processWakeupCallback, NULL, "fd_wakeup",0);

	mcwillEventAddWakeup(&s_wakeupfd_event);
    
	mcwill_event_loop();
    MPCLOG(LLV_ERROR,"mcwill_event_loop exit for errno:%d,%s", errno,strerror(errno));


    close(g_fdConnected);       
        
    toReleaseMpResource();
    sleep(2);
   
    exit(-1);
	return NULL;
}


 static void enableAckTimer(){
    if(p_ack_event==NULL){
        p_ack_event = (struct mcwill_event * )malloc(sizeof(struct mcwill_event));
        enableTimerTask(TIMER_DEFAULT_INTERVAR_FOR_SACK,
        p_ack_event,
        userTimerAckCallback,"timerAckEvent");
     }
 }
static void userTimerAckCallback(int fd, short flags, void *param,short flags2) {
   
    static int timeInt = 0;
    if(stMpdFuncs!=NULL&&stMpdFuncs->onPeriodCall!=NULL&&s_mpstatus==MP_ACTIVED){
        if(timeInt++==200){
            MPCLOG(LLV_INFO,"%s stMpdFuncs->onPeriodCall\n", __func__);
            timeInt=0;
            echoAppAllSubStatistics();
        }        
        stMpdFuncs->onPeriodCall();        
    }

    if(mpc_host.subflow_nu>0&&s_mpstatus==MP_ACTIVED&&param!=NULL)
    {
        assert(p_ack_event==param);
        enableTimerTask(TIMER_DEFAULT_INTERVAR_FOR_SACK,
            p_ack_event,
            userTimerAckCallback,"timerAckEvent");
        return ;
    }else if(p_ack_event!=NULL){
        free(p_ack_event);
        p_ack_event = NULL;
    
    }
    
}
static void echoAppAllSubStatistics(){
    int i=0;
    for(i=0;i<MAX_SUBFLOWS;i++){
        if(mpc_host.flow_name_status_key[i].status.dataFd>0&&
            (mpc_host.flow_name_status_key[i].status.statistics - mpc_host.flow_name_status_key[i].status.haveEchoedstatistics> ONE_M)){
            echoAppStreamStatistics(MESSAGE_TYPE_NOTIFY_STATICS,
                i*2==0?1:i*2,
                mpc_host.flow_name_status_key[i].status.statistics);
            mpc_host.flow_name_status_key[i].status.haveEchoedstatistics = mpc_host.flow_name_status_key[i].status.statistics;
        }

    }

}


static void processWakeupCallback(int fd, short flags, void *param,short flags2) {
	char buff[16];
	int ret;	
	do {
		ret = read(s_fdWakeupRead, buff, sizeof(buff));
	} while (ret > 0 || (ret < 0 && errno == EINTR));
}

static void mcwillEventAddWakeup(struct mcwill_event *ev) {
   
	mcwill_event_add(ev);
	triggerEvLoop();
}

static void triggerEvLoop() {

	int ret;
		
		do {
			ret = write(s_fdWakeupWrite, " ", 1);
            
		} while (ret < 0 && errno == EINTR);
	
	
}

void netLinkChangedCallback(int fd, short flags, void *param,short flags2) {

    struct sockaddr_nl addr;  
    int len;  
    char buffer[4096];  
    struct nlmsghdr *nlh;  
    len = recv(fd, buffer, 4096, 0);
    if(len>0){
        nlh = (struct nlmsghdr *)buffer;  
        while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {  
            if (nlh->nlmsg_type == RTM_NEWADDR) {  
                parseNetlinkAddrMsg(nlh, 1);  
            }else if(nlh->nlmsg_type == RTM_DELADDR){  
                parseNetlinkAddrMsg(nlh, 0);  
            }else if (nlh->nlmsg_type == RTM_NEWLINK){ 		 
		        
                parseBinaryNetlinkMessage(nlh);  
            }  
            nlh = NLMSG_NEXT(nlh, len);  
        }  
        
    }

}

void listenMcwillSocketCallback(int fd, short flags, void *param,short flags2) {

    int ret;	
	struct RecordStream *p_rs;

	struct sockaddr_un peeraddr;
	socklen_t socklen = sizeof(peeraddr);

	assert(g_fdConnected < 0);
	assert(fd == g_fdListen);
	
    MPCLOG(LLV_ERROR, "Accept mpapp connect message\r\n");
	g_fdConnected = accept(fd, (struct sockaddr *) &peeraddr, &socklen);
	if (g_fdConnected < 0) {
		MPCLOG(LLV_ERROR, "socket_ratd ACCEPT fail, %d\r\n", errno);
		mcwillEventAddWakeup(&s_listen_event);
		return;
	}
	MPCLOG(LLV_INFO, "socket_ratd ACCEPT succeed, %d\r\n", g_fdConnected);
	ret = fcntl(g_fdConnected, F_SETFL, O_NONBLOCK); 

	if (ret < 0) {	
		MPCLOG(LLV_ERROR, "set g_fdConnected nonblock fail..\n");		
	}

	p_rs = record_stream_new(g_fdConnected, MAX_COMMAND_BYTES);

	mcwill_event_set(&s_commands_event, g_fdConnected, true,
			processAppCommandsCallback, p_rs, "fd_mpapp",0);

	mcwillEventAddWakeup(&s_commands_event);
    MPCLOG(LLV_ERROR, "Finished mpapp<----->mpc\r\n");
    //setWatchDog();
    mcwill_startKeepAliveCheckTask();
   
}


int echoAppException(__be32 exceptionCode,__be32 netType){

    if(exceptionCode == EXCEPTION_CODE_MP_IDLE){
        s_mpstatus=MP_IDLE;
        MPCLOG(LLV_ERROR, " set mp to IDLE status \n");
    }
    u_char * rsp = encodeExceptionRsp(MESSAGE_TYPE_EXCEPTION,exceptionCode,netType);
    sendResponseRaw(rsp,strlen(rsp));
    free(rsp);
    return 0;
}

int echoAppResult(__be32 messageType,__be32 sessionId,__be32 result){

    u_char * rsp = encodeAppRsp(messageType,sessionId,result);
    sendResponseRaw((void *)rsp,strlen(rsp));
    free(rsp);
    return 0;
}


int echoAppKeepAlive(__be32 messageType,__be32 sessionId){

    u_char * rsp = encodeAppTypeAndSessionId(messageType,sessionId);
    sendResponseRaw((void *)rsp,strlen(rsp));
    free(rsp);
    return 0;
}


int echoAppResultWithNetType(__be32 messageType,__be32 sessionId,__be32 netType,__be32 result){

                      
    u_char * rsp = encodeAppRspWithNetType(messageType,sessionId,netType,result);
    sendResponseRaw(rsp,strlen(rsp));
    free(rsp);
    return 0;
}
int echoAppStreamStatistics(__be32 messageType,__be32 netType,__be64 statistics){

    u_char * rsp = encodeAppRspStatistics(messageType,netType,statistics);
    sendResponseRaw(rsp,strlen(rsp));
    free(rsp);
    return 0;
}

int sendResponseRaw(const void *data, size_t dataSize) {

	int fd = g_fdConnected;	

	if (fd < 0) {	        
		return -1;
	}
	int ret;

	if (dataSize > MAX_COMMAND_BYTES) {
		return -1;
	}

	pthread_mutex_lock(&s_writeMutex);

	ret = blockingWrite(fd, data, dataSize);

	if (ret < 0) {
		pthread_mutex_unlock(&s_writeMutex);        
		return ret;
	}

	pthread_mutex_unlock(&s_writeMutex);

	return 0;
}

int blockingWrite(int fd, const void *buffer, size_t len) {

	size_t writeOffset = 0;
	const uint8_t *toWrite;
	toWrite = (const uint8_t *) buffer;    

	while (writeOffset < len) {
		ssize_t written;
		do {
			written = write(fd, toWrite + writeOffset, len - writeOffset);            
		} while (written < 0 && errno == EINTR);

		if (written >= 0) {
			writeOffset += written;
		} else { 
			
			mcwill_event_del(&s_commands_event,&g_fdConnected);
			mcwillEventAddWakeup(&s_listen_event);
			return -1;
		}
	}    
	return 0;
}


void initDefaultParams(){

    
    g_keepAliveinfo = 0l;
    memset(&mpc_op, 0, sizeof(mpc_op));
      
    mpc_op.dns_addr = "172.16.251.77";
    mpc_op.svr_addr = "103.5.126.153";
    
    mpc_op.svr_data_port = MP_DATA_PORT;
    mpc_op.svr_command_port = MP_COMMAND_PORT;   

    mpc_op.table_name = "250";
    
    memset(&mpc_host, 0, sizeof(struct MPC_HOST));

    mpc_host.subflow_nu=0;
    
    mpc_host.dev="tun0";   
   
    memcpy(mpc_host.flow_name_status_key,_flow_name_status_key, 3*sizeof(struct subFlowMap));
    
}

int mcwill_socket_init() {

   

    int ret;
    int flags;
    g_fdListen = android_get_control_socket(SOCKET_NAME);
    if (g_fdListen < 0) {
        MPCLOG(LLV_ERROR,"android_get_control_socket(%s) failed. %s(%d)\n",
                SOCKET_NAME,strerror(errno), errno);
        return -1;
    } else {
        
    }
    ret = listen(g_fdListen, 4);

    if (ret < 0) {
        
        return -1;
    }
    return 0;

}

int netlink_socket_init(){

        struct sockaddr_nl addr;  
        int len;  
        char buffer[4096];  
        struct nlmsghdr *nlh;
      
        if ((g_fdNetlink = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {  
            perror("couldn't open NETLINK_ROUTE socket");  
            return -1;  
        }  
      
        memset(&addr, 0, sizeof(addr));  
        addr.nl_family = AF_NETLINK;        
        addr.nl_groups =RTMGRP_LINK; 
        if (bind(g_fdNetlink, (struct sockaddr *)&addr, sizeof(addr)) == -1) {  
            MPCLOG(LLV_ERROR,"couldn't bind\n"); 
            return -1;  
        }  
        
    return 0;

}


void parseBinaryNetlinkMessage(struct nlmsghdr *nh) {  
    int len = nh->nlmsg_len - sizeof(*nh);  
    struct ifinfomsg *ifi;  
  
    if (sizeof(*ifi) > (size_t) len) {  
        MPCLOG(LLV_INFO,"Got a short RTM_NEWLINK message\n");  
        return;  
    }  
  
    ifi = (struct ifinfomsg *)NLMSG_DATA(nh);  
    if ((ifi->ifi_flags & IFF_LOOPBACK) != 0) {  
        return;  
    }  
  
    struct rtattr *rta = (struct rtattr *)  
      ((char *) ifi + NLMSG_ALIGN(sizeof(*ifi)));  
    len = NLMSG_PAYLOAD(nh, sizeof(*ifi));  
  
    while(RTA_OK(rta, len)) {  
        switch(rta->rta_type) {  
          case IFLA_IFNAME:  
          {  
            char ifname[IFNAMSIZ];  
            char *action;  
            snprintf(ifname, sizeof(ifname), "%s",(char *) RTA_DATA(rta)); 
           
            bool isUp= (ifi->ifi_flags & IFF_RUNNING)
                &&(ifi->ifi_flags &IFF_UP)
                &&(ifi->ifi_flags &IFF_LOWER_UP);
            
           

            bool isDown= !(ifi->ifi_flags & IFF_RUNNING)
                &&!(ifi->ifi_flags &IFF_UP);

            
            if((isUp||isDown)&&!strcasecmp(ifname,MP_SUBFLOW_NAME_WIFI)||!strcasecmp(ifname,MP_SUBFLOW_NAME_MCWILL)
                ||!strcasecmp(ifname,"tun0")||!strcasecmp(ifname,MP_SUBFLOW_NAME_4G)){
                
                int netType=0;
                if(!strcasecmp(ifname,MP_SUBFLOW_NAME_WIFI))netType=MP_STREAM_FLAG_WIFI;
                else if(!strcasecmp(ifname,MP_SUBFLOW_NAME_MCWILL))netType=MP_STREAM_FLAG_MCWILL ;
                else if(!strcasecmp(ifname,MP_SUBFLOW_NAME_4G))netType=MP_STREAM_FLAG_4G;
                else{}
                
                if(netType!=0){
                    bool newStatus = (isUp?true:false);
                    if(g_netStatus[netType/2]!=newStatus){
                        MPCLOG(LLV_ERROR," network_status_change  %s link %s\n", ifname, newStatus?"up":"down");
                        g_netStatus[netType/2]=newStatus;
                    }
                     
                }
               
            }
          }  
        }  
  
        rta = RTA_NEXT(rta, len);  
    }  
}  
  
void parseNetlinkAddrMsg(struct nlmsghdr *nlh, int newStatus)  
{  
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);  
    struct rtattr *rth = IFA_RTA(ifa);  
    int rtl = IFA_PAYLOAD(nlh);  
  
    while (rtl && RTA_OK(rth, rtl)) {  
        if (rth->rta_type == IFA_LOCAL) {  
            uint32_t ipaddr = htonl(*((uint32_t *)RTA_DATA(rth)));  
            char name[IFNAMSIZ];  
            if_indextoname(ifa->ifa_index, name);  
            if(!strcasecmp(name,MP_SUBFLOW_NAME_WIFI)||!strcasecmp(name,MP_SUBFLOW_NAME_MCWILL)
                    ||!strcasecmp(name,"tun0")||!strcasecmp(name,MP_SUBFLOW_NAME_4G)){

                int netType=0;
                if(!strcasecmp(name,MP_SUBFLOW_NAME_WIFI))netType=MP_STREAM_FLAG_WIFI;
                else if(!strcasecmp(name,MP_SUBFLOW_NAME_MCWILL))netType=MP_STREAM_FLAG_MCWILL;
                else if(!strcasecmp(name,MP_SUBFLOW_NAME_4G))netType=MP_STREAM_FLAG_4G;
                else{}
               
                if(netType!=0){
                    g_netStatus[netType/2]=(newStatus != 0?true:false);

                }               
            }
        }  
        rth = RTA_NEXT(rth, rtl);  
    }  
}  




void normalDataRecvCallback(int fd, short flags, void *param,short netType){

    int fdIndex = (int *)param;
   
    char *buf;
    register int len;
    struct mp_datas_head hdr;
   
    if( !(buf = malloc(MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE )) ){
        MPCLOG(LLV_ERROR,"Can't allocate buffer for the normalDataRecvCallback\n"); 
        return ; 
    }
  
    MPCLOG(LLV_INFO,"normalDataRecvCallback   malloc buf = %p,size = %d\n",buf,MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE); 
    memset(buf,0x00,MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE);
   
    if( (len=udp_read(fd, buf,MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE)) <= 0 ){
        
        MPCLOG(LLV_ERROR,"udp_read error, free buf %p \n",buf);
        free(buf);buf=NULL;
        return ;
    }
    if((len-sizeof(struct mp_datas_head))>0)
        switchMPToActivite();

    mpc_host.flow_name_status_key[fdIndex].status.statistics+=len;
    if( (len-sizeof(struct mp_datas_head))>=0 && mp_data_recv_to_transport(mpc_host.loc_fd,buf,len) < 0 ){
        if(errno==EINVAL){
            mcwill_event_del(&(mpc_host.flow_name_status_key[fdIndex].status.data_event),&mpc_host.flow_name_status_key[fdIndex].status.dataFd);
            triggerEvLoop();
        }
    }else{
        if((len-sizeof(struct mp_datas_head))<0&&buf!=NULL){
            
            free(buf);
            buf = NULL;
        }
    }

}



void commandDataRecvCallback(int fd, short flag, void *param,short netType){

    struct magRsp rsp;
    memset(&rsp, 0, sizeof(rsp));

    struct RecordStream *p_rs;
	void *p_record;
	size_t recordlen;
	int ret;

	p_rs = (struct RecordStream *) param;
     
	for (;;) {
		
		ret = record_stream_get_next(p_rs, &p_record, &recordlen);
        
		if (ret == 0 && p_record == NULL) {
			break;
		} else if (ret < 0) {
			break;
		} else if (ret == 0&& p_record != NULL) { 			
            if(decodeMagCommandBuffer(&rsp,p_record, recordlen,netType)){

                
                switch(rsp.messageType){
                    case MESSAGE_TYPE_ADD_SUBFLOW_RSP:{
                                        
                        if(rsp.result==RESULT_OK){
                            
                            int fdIndex = netType/2;
                            //maybe same result echo twice
                            if(mpc_host.flow_name_status_key[fdIndex].status.dataFd<0){
                                int fd_tmp = establishUdpSession(&mpc_host,mpc_host.flow_name_status_key[fdIndex].netTypeName,false);
                                         
                                if (fd_tmp == -1){
                                    MPCLOG(LLV_ERROR,"       	<<< :%s, %s,result = %d\n",GetMessageInfo(rsp.messageType),GetNetType(netType/2),RESULT_FAILED);

                                    echoAppResultWithNetType(rsp.messageType,rsp.sessionId,netType,RESULT_FAILED);

                                    return ;
                                }

                                //MPCLOG(LLV_ERROR,"mcwill_event add rsp mpc_host.flow_name_status_key[%d].status.dataFd= %d,Stream = %s\n",fdIndex,fd_tmp,GetNetType(fdIndex));


                                mpc_host.flow_name_status_key[fdIndex].status.dataFd = fd_tmp;
                                mcwill_event_set(&(mpc_host.flow_name_status_key[fdIndex].status.data_event),
                                       mpc_host.flow_name_status_key[fdIndex].status.dataFd,
                                       true,
                    			        normalDataRecvCallback,
                    			        fdIndex,
                    			        "-data-",netType);
                                mcwillEventAddWakeup(&(mpc_host.flow_name_status_key[fdIndex].status.data_event));

                            }


                            if(s_mp_dispatch[fdIndex]==0){
                                task_queue(fdIndex,task_mpMsgQueueDispatch,10);
                            }else{
                                MPCLOG(LLV_INFO, "task %p , is already running...\n",s_mp_dispatch[fdIndex]);
                                Q_MP_DISPATCH[fdIndex].quit=false;
                                notifyMpdNetStatus(netType,NETWORK_MP_ENABLE);
                            }

                            mpc_host.flow_name_status_key[fdIndex].status.sub_mp_status = NETWORK_MP_ENABLE;

                            int lastNum = mpc_host.subflow_nu;
                            updateCurrentSubFlowsNum();

                            if(lastNum==0&&mpc_host.subflow_nu==1){

                                if(mpc_host.loc_fd<=0){
                                    if(init_tun_network(&mpc_host)<0){
                                           MPCLOG(LLV_ERROR,"       	<<< :%s, %s,result = %d\n",GetMessageInfo(rsp.messageType),GetNetType(netType/2),RESULT_FAILED);

                                           echoAppResultWithNetType(rsp.messageType,rsp.sessionId,netType,RESULT_FAILED);

                                            return ;
                                    }
                                }
                                    clearMpRouteAndIptables();
                                    addMpRouteAndIptables();
                                    s_mpstatus = MP_ACTIVED;

                                   // mcwill_event_del(&s_tun_event);// delete
                                    mcwill_event_set(&s_tun_event, mpc_host.loc_fd, true,
                                    			tunDataRecvCallback, NULL, "fd_tun",0);

                                    mcwillEventAddWakeup(&s_tun_event);

                                    enableAckTimer();



                        }


                        if(mpc_host.subflow_nu>MAX_SUBFLOWS){
                            mpc_host.subflow_nu = MAX_SUBFLOWS;
                            MPCLOG(LLV_ERROR,"There have some error on available subflows ,you should check mp-app(subFlow >3 )  \n");
                        }

                        MPCLOG(LLV_ERROR,"       	<<< :%s, %s,result = %d\n",GetMessageInfo(rsp.messageType),GetNetType(netType/2),RESULT_OK);


                        echoAppResultWithNetType(rsp.messageType,rsp.sessionId,netType,RESULT_OK);




                        }
                    }
                        break;

                    case MESSAGE_TYPE_TRY_SUBFLOW_RSP:{

                         memset(&(mpc_host.detectTimeInfo[netType/2]),0,sizeof(struct sysinfo ));


                         MPCLOG(LLV_ERROR,"       	<<< :%s, %s,%d\n",GetMessageInfo(rsp.messageType),GetNetType(netType/2),rsp.result);

                         echoAppResultWithNetType(rsp.messageType,rsp.sessionId,netType,rsp.result);
                    }
                        break;
                    case MESSAGE_TYPE_RELEASE_MP_RSP:{
                         echoAppResult(rsp.messageType,rsp.sessionId,rsp.result);
                         MPCLOG(LLV_ERROR,"       	<<< :%s,%s\n",GetMessageInfo(MESSAGE_TYPE_RELEASE_MP_RSP),rsp.result==RESULT_OK?"true":"false");
                         if(rsp.result==RESULT_OK){

                            toReleaseMpResource();


                         }
                    }
                        break;
                    case MESSAGE_TYPE_SYNC_MP_INIT_RSP:{
                        echoAppResult(MESSAGE_TYPE_ENABLE_MP_RSP,rsp.sessionId,rsp.result);
                        MPCLOG(LLV_ERROR,"       	<<< :%s,%s\n",GetMessageInfo(MESSAGE_TYPE_ENABLE_MP_RSP),rsp.result==RESULT_OK?"true":"false");
                        if(rsp.result==RESULT_OK){

                        }
                    }
                        break;
                }


            }
            memset(p_rs->buffer,0,p_rs->maxRecordLen+HEADER_SIZE);

		}
	}

	if (ret == 0 || !(errno == EAGAIN || errno == EINTR || errno == 0)) {

		record_stream_free(p_rs);

	}

}

void tunDataRecvCallback(int fd, short flags, void *param,short flags2){
    char *buf;
    register int len;

    if( !(buf = malloc(MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE)) ){
        MPCLOG(LLV_ERROR,"Can't allocate buffer for the tunDataRecvCallback\n");
        return ;
    }

    MPCLOG(LLV_INFO,"============TunDataRecvCallback buf = %p,size = %d======\n",buf,MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE);

    memset(buf,0x00,MPC_MAX_MP_HEAD_LEN+MPC_FRAME_SIZE);
    if( (len = tun_read(fd, buf+MPC_MAX_MP_HEAD_LEN, MPC_FRAME_SIZE)) < 0 ){
    }
    if( len==0 ){
        free(buf);buf = NULL;
        return;
    }else {
        len = len+MPC_MAX_MP_HEAD_LEN;
    }

    switchMPToActivite();

    if( len-MPC_MAX_MP_HEAD_LEN>0 && mp_data_send(buf, len) < 0 ){
        MPCLOG(LLV_INFO,"           Recv tun data,but call mp_data_send Error,(mpd onSendNewTransData Failed)\n");

    }else {
        if(len-MPC_MAX_MP_HEAD_LEN<=0&&buf!=NULL){

            free(buf);buf = NULL;
        }

    }

}


void processAppCommandsCallback(int fd, short flags, void *param,short flags2) {
	struct RecordStream *p_rs;
	void *p_record;
	size_t recordlen=0;
	int ret;
	p_rs = (struct RecordStream *) param;
    MPCLOG(LLV_ERROR, "receive app command \n");
	for (;;) {
		ret = record_stream_get_next(p_rs, &p_record, &recordlen);
        MPCLOG(LLV_INFO, "record_stream_get_next ret = %d,p_record = %p,recordlen = %d \n",ret,p_record,recordlen);
		if (ret == 0 && p_record == NULL) {
            MPCLOG(LLV_ERROR, "read end-of-stream for socket_ratd(app) \n");

            clearMpRouteAndIptables();
            mcwill_event_del(&s_tun_event,&mpc_host.loc_fd);
            triggerEvLoop();

            //echoAppException(EXCEPTION_CODE_MP_TUN_CLOSE, MP_STREAM_FLAG_TUN0);
            //tun_close(mpc_host.loc_fd,mpc_host.dev);
            //mpc_host.loc_fd = -1;
            s_mpstatus = MP_DISACTIVE;
            removeAllSubFlowsEvent();

			break;
		} else if (ret < 0) {
			break;
		} else if (ret == 0&& p_record  != NULL) {

            processAppCommandBuffer(p_record, recordlen);

            memset(p_rs->buffer,0,p_rs->maxRecordLen+HEADER_SIZE);

		}
	}

	if (ret == 0 || !(errno == EAGAIN || errno == EINTR || errno == 0)) {
		MPCLOG(LLV_ERROR, "maybe mp-app client have been closed ( error = %d, %d)\n", ret, errno);


		mcwill_event_del(&s_commands_event,&g_fdConnected);
		record_stream_free(p_rs);
		mcwillEventAddWakeup(&s_listen_event);
	}

}

int processAppCommandBuffer(void *buffer, size_t buflen) {


    void *appCommand=NULL;

    decodeAppCommandBufferNew(&appCommand,buffer,buflen);
    if(appCommand!=NULL){
        MPCLOG(LLV_ERROR, ">>> : %s\n",GetMessageInfo(((struct r_base*)appCommand)->messageType));

        g_keepAliveinfo = myUptimeMillis();

        //MPCLOG(LLV_ERROR, ">>> : %s, sessionId = %d\n",GetMessageInfo(((struct r_base*)appCommand)->messageType),((struct r_base*)appCommand)->sessionId);
        switch(((struct r_base*)appCommand)->messageType){

            case MESSAGE_TYPE_CONFIG_MP_REQ:{
                struct r_app_config r_config;
                memcpy(&r_config, appCommand, sizeof(struct r_app_config));

                free((struct r_app_config*)appCommand);

                mpc_op.uid      =  r_config.uid;
                mpc_op.dns_addr =  strdup(r_config.dns);
                mpc_op.svr_addr =  strdup(r_config.mag);



                stMpdFuncs = mpd_Init(mpc_op.uid,&s_mpcEnv);

                MPCLOG(LLV_ERROR, "mpc Version : %s,mpd Version :  %s  \r\n\r\n",MPC_VERSION,stMpdFuncs->getVersion());
                echoAppResult(MESSAGE_TYPE_CONFIG_MP_RSP,r_config.baseInfo.sessionId,RESULT_OK);

            }
                break;

            case MESSAGE_TYPE_ENABLE_MP_REQ:{

                struct r_app_enable r_enable;
                memcpy(&r_enable, appCommand, sizeof(struct r_app_enable));

                free((struct r_app_enable*)appCommand);


                notifyMpdNetStatus(MP_STATE,NETWORK_UP);


                struct r_sync_mp_init r_mp_init;

                r_mp_init.baseInfo.messageType = MESSAGE_TYPE_SYNC_MP_INIT_REQ;
                r_mp_init.baseInfo.sessionId   = r_enable.baseInfo.sessionId;
                r_mp_init.uid = mpc_op.uid;
                r_mp_init.numberType = MP_STATE;


                int fdIndex = r_enable.netType/2;
                if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){

                    //close(mpc_host.flow_name_status_key[fdIndex].status.commandFd);
                    //mpc_host.flow_name_status_key[fdIndex].status.commandFd= -1;
                    mcwill_event_del(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),&mpc_host.flow_name_status_key[fdIndex].status.commandFd);
                    triggerEvLoop();
                }

                //if(mpc_host.flow_name_status_key[fdIndex].status.commandFd<0)
                {

                    int fd = establishUdpSession(&mpc_host,mpc_host.flow_name_status_key[fdIndex].netTypeName,true);

                    if (fd == -1){
                        MPCLOG(LLV_ERROR,"can't establish command session on %s for mp init state \n",mpc_host.flow_name_status_key[fdIndex].netTypeName);

                        echoAppResult(MESSAGE_TYPE_ENABLE_MP_RSP,
                            r_enable.baseInfo.sessionId,RESULT_FAILED);
                        MPCLOG(LLV_ERROR,"       	<<< :%s,failed\n",GetMessageInfo(MESSAGE_TYPE_ENABLE_MP_RSP));
                        return 0;
                    }
                    //MPCLOG(LLV_ERROR,"mcwill_event enable mp req mpc_host.flow_name_status_key[%d].status.commandFd= %d,Stream = %s\n",fdIndex,fd,GetNetType(fdIndex));

                    mpc_host.flow_name_status_key[fdIndex].status.commandFd = fd;

                    struct RecordStream *p_rs;

                    p_rs = record_stream_new(mpc_host.flow_name_status_key[fdIndex].status.commandFd, MAX_COMMAND_BYTES);


                    mcwill_event_set(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),
                                    mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                    true,
                			        commandDataRecvCallback,
                			        p_rs,
                			        "-command-",r_enable.netType);
                    mcwillEventAddWakeup(&(mpc_host.flow_name_status_key[fdIndex].status.command_event));
                 }

                u_char * remote_req = encodeMPSyncInitRequest(&r_mp_init);

                int res = udp_write(mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                remote_req,LEN_TYPE_SYNC_MP_INIT_REQ);
                if(res<0){

                    if(res==-1){
                        echoAppException(EXCEPTION_CODE_MP_SUBFLOW_ERROR, r_enable.netType);

                    }

                     echoAppResult(MESSAGE_TYPE_ENABLE_MP_RSP,
                            r_enable.baseInfo.sessionId,RESULT_FAILED);
                        MPCLOG(LLV_ERROR,"       	<<< :%s,failed\n",GetMessageInfo(MESSAGE_TYPE_ENABLE_MP_RSP));

                }
                free(remote_req);

            }

                break;
            case MESSAGE_TYPE_DISABLE_MP_REQ:{

                struct r_app_disable r_disable;
                memcpy(&r_disable, appCommand, sizeof(struct r_app_disable));

                free((struct r_app_disable*)appCommand);

                toReleaseMpResource();

                echoAppResult(MESSAGE_TYPE_DISABLE_MP_RSP,r_disable.baseInfo.sessionId,RESULT_OK);
                MPCLOG(LLV_ERROR,"       	<<< :%s\n",GetMessageInfo(MESSAGE_TYPE_DISABLE_MP_RSP));

            }
                break;
            case MESSAGE_TYPE_RELEASE_MP_REQ:{

                struct r_app_release_mp r_release;
                memcpy(&r_release, appCommand, sizeof(struct r_app_release_mp));
                free((struct r_app_release_mp*)appCommand);

                if(mpc_host.subflow_nu <= 0){
                    echoAppResultWithNetType(MESSAGE_TYPE_RELEASE_MP_RSP,r_release.baseInfo.sessionId,r_release.numberType,RESULT_FAILED);

                    MPCLOG(LLV_ERROR, "no subflow is running on Multi-path\n");
                    MPCLOG(LLV_ERROR,"       	<<< :%s\n",GetMessageInfo(MESSAGE_TYPE_RELEASE_MP_RSP));

                    return -1;
                }

                u_char * remote_req = encodeMPReleaseRequest(&r_release);

                sendCommandMsgOnAllSubflows(remote_req,LEN_TYPE_RELEASE_MP_REQ);

                free(remote_req);

             }
                break;
            case MESSAGE_TYPE_ADD_SUBFLOW_REQ:{

                struct r_app_add r_add;
                memcpy(&r_add, appCommand, sizeof(struct r_app_add));

                free((struct r_app_add_del*)appCommand);

                int fdIndex = r_add.netType/2;

                if(mpc_host.flow_name_status_key[fdIndex].status.commandFd<=0){

                    int fd = establishUdpSession(&mpc_host,mpc_host.flow_name_status_key[fdIndex].netTypeName,true);

                    if (fd == -1){
                        MPCLOG(LLV_ERROR,"can't establish data command session on %s \n",mpc_host.flow_name_status_key[fdIndex].netTypeName);
                        MPCLOG(LLV_ERROR,"       	<<< :%s, %s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_ADD_SUBFLOW_RSP),GetNetType(r_add.netType/2),RESULT_FAILED);

                        echoAppResultWithNetType(MESSAGE_TYPE_ADD_SUBFLOW_RSP,r_add.baseInfo.sessionId,r_add.netType,RESULT_FAILED);
                        return 0;
                    }
                    mpc_host.flow_name_status_key[fdIndex].status.commandFd = fd;
                    struct RecordStream *p_rs;

                    p_rs = record_stream_new(mpc_host.flow_name_status_key[fdIndex].status.commandFd, MAX_COMMAND_BYTES);



                    mcwill_event_set(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),
                                    mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                    true,
                			        commandDataRecvCallback,
                			        p_rs,
                			        "-command-",r_add.netType);

                    mcwillEventAddWakeup(&(mpc_host.flow_name_status_key[fdIndex].status.command_event));

                 }

                 u_char * remote_req = encodeAddRequest(&r_add);


                if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){
                    int res = udp_write(mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                remote_req,LEN_TYPE_ADD_SUBFLOW_REQ);

                    if(res>=0){
                        break ;
                     }else if(res==-1){
                             echoAppException(EXCEPTION_CODE_MP_SUBFLOW_ERROR, r_add.netType);

                     }


                }
                MPCLOG(LLV_ERROR,"       	<<< :%s, %s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_ADD_SUBFLOW_RSP),GetNetType(r_add.netType/2),RESULT_FAILED);

                echoAppResultWithNetType(MESSAGE_TYPE_ADD_SUBFLOW_RSP,ntohl(r_add.baseInfo.sessionId),r_add.netType,RESULT_FAILED);


                free(remote_req);
            }
                break;
            case MESSAGE_TYPE_DEL_SUBFLOW_REQ:{

                struct r_app_del r_del;
                memcpy(&r_del, appCommand, sizeof(struct r_app_del));
                free((struct r_app_del*)appCommand);
                if(mpc_host.subflow_nu <= 0){
                    echoAppResultWithNetType(MESSAGE_TYPE_DEL_SUBFLOW_RSP,ntohl(r_del.baseInfo.sessionId),r_del.numberType,RESULT_FAILED);

                    MPCLOG(LLV_ERROR, "no subflow is running on Multi-path\n");
                    MPCLOG(LLV_ERROR,"       	<<< :%s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_DEL_SUBFLOW_RSP),RESULT_FAILED);

                    return -1;
                }

                u_char * remote_req = encodeDelRequest(&r_del);

                sendCommandMsgOnAllSubflows(remote_req,LEN_TYPE_DEL_SUBFLOW_REQ);

                removeSubFlowsEvent(r_del.numberType);

                finishsubflowDispathTask(r_del.numberType);
                if(MP_STREAM_FLAG_4G&r_del.numberType){
                    notifyMpdNetStatus(MP_STREAM_FLAG_4G,NETWORK_MP_DISABLE);
                }

                if(MP_STREAM_FLAG_MCWILL&r_del.numberType){
                    notifyMpdNetStatus(MP_STREAM_FLAG_MCWILL,NETWORK_MP_DISABLE);
                }

                if(MP_STREAM_FLAG_WIFI&r_del.numberType){
                    notifyMpdNetStatus(MP_STREAM_FLAG_WIFI,NETWORK_MP_DISABLE);
                }

                MPCLOG(LLV_ERROR,"       	<<< :%s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_DEL_SUBFLOW_RSP),RESULT_OK);


                echoAppResultWithNetType(MESSAGE_TYPE_DEL_SUBFLOW_RSP,ntohl(r_del.baseInfo.sessionId),r_del.numberType,RESULT_OK);



                if(mpc_host.subflow_nu==0){
                    mcwill_event_del(&s_tun_event,&mpc_host.loc_fd);
                    triggerEvLoop();
                    echoAppException(EXCEPTION_CODE_MP_TUN_CLOSE, MP_STREAM_FLAG_TUN0);
     			    clearMpRouteAndIptables();
    			}

                free(remote_req);
            }
        	break;

            case MESSAGE_TYPE_TRY_SUBFLOW_REQ:{

                struct r_app_try r_try;
                memcpy(&r_try, appCommand, sizeof(struct r_app_try));
                free((struct r_app_try*)appCommand);

                int fdIndex = r_try.netType/2;
                struct sysinfo cur_time;
                memset(&cur_time,0,sizeof(struct sysinfo ));
                //sysinfo(&cur_time);

                if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){

                    if((mpc_host.detectTimeInfo[fdIndex]).uptime!=0
                        &&!sysinfo(&cur_time)
                        &&(cur_time.uptime-(mpc_host.detectTimeInfo[fdIndex]).uptime>10)){

                        mcwill_event_del(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),&mpc_host.flow_name_status_key[fdIndex].status.commandFd);
                        triggerEvLoop();
                        sysinfo(&(mpc_host.detectTimeInfo[fdIndex]));
                     }else if((mpc_host.detectTimeInfo[fdIndex]).uptime==0){
                        sysinfo(&(mpc_host.detectTimeInfo[fdIndex]));
                     }

                }



                if(mpc_host.flow_name_status_key[fdIndex].status.commandFd<0){

                    int fd = establishUdpSession(&mpc_host,mpc_host.flow_name_status_key[fdIndex].netTypeName,true);

                    if (fd == -1){
                        MPCLOG(LLV_ERROR,"can't establish command session on %s \n",mpc_host.flow_name_status_key[fdIndex].netTypeName);

                        MPCLOG(LLV_ERROR,"       	<<< :%s,%s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_TRY_SUBFLOW_RSP),GetNetType(r_try.netType/2),RESULT_FAILED);

                        echoAppResultWithNetType(MESSAGE_TYPE_TRY_SUBFLOW_RSP,r_try.baseInfo.sessionId,r_try.netType,RESULT_FAILED);
                        return 0;
                    }
                    mpc_host.flow_name_status_key[fdIndex].status.commandFd = fd;

                    struct RecordStream *p_rs;

                    p_rs = record_stream_new(mpc_host.flow_name_status_key[fdIndex].status.commandFd, MAX_COMMAND_BYTES);

                    mcwill_event_set(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),
                                    mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                    true,
                			        commandDataRecvCallback,
                			        p_rs,
                			        "-command-",r_try.netType);

                    mcwillEventAddWakeup(&(mpc_host.flow_name_status_key[fdIndex].status.command_event));


                 }

                unsigned short rttd = stMpdFuncs->getRTTD(r_try.netType); ;
                u_char * remote_req = encodeTryRequest(&r_try,rttd);
                int res = udp_write(mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                                remote_req,LEN_TYPE_TRY_SUBFLOW_REQ);
                if(res<0){
                    if(res==-1){
                        echoAppException(EXCEPTION_CODE_MP_SUBFLOW_ERROR, r_try.netType);

                    }
                     MPCLOG(LLV_ERROR,"       	<<< :%s,%s,result = %d\n",GetMessageInfo(MESSAGE_TYPE_TRY_SUBFLOW_RSP),GetNetType(r_try.netType/2),RESULT_FAILED);

                     echoAppResultWithNetType(MESSAGE_TYPE_TRY_SUBFLOW_RSP,ntohl(r_try.baseInfo.sessionId),r_try.netType,RESULT_FAILED);
                }
                free(remote_req);

            }
                break;
            case MESSAGE_TYPE_KEEPALIVE_REQ:{
                struct r_app_mpc_keepalive r_keepalive;
                memcpy(&r_keepalive, appCommand, sizeof(struct r_app_mpc_keepalive));

                free((struct r_app_mpc_keepalive*)appCommand);



                MPCLOG(LLV_ERROR,"       	<<< :%s\n",GetMessageInfo(MESSAGE_TYPE_KEEPALIVE_RSP));

                echoAppKeepAlive(MESSAGE_TYPE_KEEPALIVE_RSP,r_keepalive.baseInfo.sessionId);

                s_watchDog_reset = false;
            
                
            }
                break;

            case MESSAGE_TYPE_ENABLE_MPC_LOG:{
             
                struct r_app_enable_log r_log;
                memcpy(&r_log, appCommand, sizeof(struct r_app_enable_log));
                free((struct r_app_enable_log*)appCommand);
                
                g_log_enable = r_log.operation;
                
                
                
            }
                break;


            case MESSAGE_TYPE_SYNC_MP_INIT_REQ:{
                notifyMpdNetStatus(MP_STATE,NETWORK_UP);
                                         
             }
                break;
            default:
                MPCLOG(LLV_INFO, "~~~~~~~~~~~~~~~~~~ (command not recognized) --- messageType = %d\n",((struct r_base*)appCommand)->messageType);
                break;
        }

    }
    return 0;
}

int sendCommandMsgOnAllSubflows(char *buffer ,int len){

    int fdIndex = 0;

    for(fdIndex =0;fdIndex<3;fdIndex++){
	    if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){
         int res =udp_write(mpc_host.flow_name_status_key[fdIndex].status.commandFd,
                            buffer,len);
         if(res==-1){
            
            mcwill_event_del(&(mpc_host.flow_name_status_key[fdIndex].status.command_event),&mpc_host.flow_name_status_key[fdIndex].status.commandFd);
            mcwill_event_del(&(mpc_host.flow_name_status_key[fdIndex].status.data_event),&mpc_host.flow_name_status_key[fdIndex].status.dataFd);
            triggerEvLoop();
          }
	    }
    }
    return 0;
}


void removeSubFlowsEvent(__u8 numberType){

    int fdIndex = -1;
    
    if(MP_STREAM_FLAG_4G&numberType){
        fdIndex = MP_STREAM_FLAG_4G/2;
    }

    if(MP_STREAM_FLAG_MCWILL&numberType){
        fdIndex = MP_STREAM_FLAG_MCWILL/2;
    }

    if(MP_STREAM_FLAG_WIFI&numberType){
        fdIndex = MP_STREAM_FLAG_WIFI/2;
    }
    if(fdIndex!=-1&&fdIndex>=0&&fdIndex<MAX_SUBFLOWS){
        removeSpecifiedSubFlow(fdIndex); 
    }
  
}

void removeSpecifiedSubFlow(int fdIndex){
    
    Q_MP_DISPATCH[fdIndex].quit=true;
    mpc_host.flow_name_status_key[fdIndex].status.sub_mp_status = NETWORK_MP_DISABLE;
    updateCurrentSubFlowsNum();
    mpc_host.flow_name_status_key[fdIndex].RTTC = 0;

    if(mpc_host.flow_name_status_key[fdIndex].status.dataFd>0){
        mcwill_event_del(&mpc_host.flow_name_status_key[fdIndex].status.data_event,&mpc_host.flow_name_status_key[fdIndex].status.dataFd);
        triggerEvLoop();
        
    }
    if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){
        mcwill_event_del(&mpc_host.flow_name_status_key[fdIndex].status.command_event,&mpc_host.flow_name_status_key[fdIndex].status.commandFd);
       
        triggerEvLoop();
    }

}

int removeAllSubFlowsEvent(){

    int fdIndex = 0;

    for(fdIndex =0;fdIndex<3;fdIndex++){
        if(mpc_host.flow_name_status_key[fdIndex].status.dataFd>0){
            mcwill_event_del(&mpc_host.flow_name_status_key[fdIndex].status.data_event,&mpc_host.flow_name_status_key[fdIndex].status.dataFd);
            
            triggerEvLoop();
        }
        if(mpc_host.flow_name_status_key[fdIndex].status.commandFd>0){

            mcwill_event_del(&mpc_host.flow_name_status_key[fdIndex].status.command_event,&mpc_host.flow_name_status_key[fdIndex].status.commandFd);
            triggerEvLoop();
           
        }
        Q_MP_DISPATCH[fdIndex].quit=true;
        mpc_host.flow_name_status_key[fdIndex].status.haveEchoedstatistics= 0;
        mpc_host.flow_name_status_key[fdIndex].status.statistics = 0;
        mpc_host.flow_name_status_key[fdIndex].status.sub_mp_status = NETWORK_MP_DISABLE;
        mpc_host.flow_name_status_key[fdIndex].RTTC = 0;
    }
    mpc_host.subflow_nu=0;
    return 0;
}

int enableTimerTask_second(int interval,struct mcwill_event * ev,mcwill_event_cb func,char evname[]){

   
    if(!mcwill_timer_check_exist(ev)){
        struct timeval myRelativeTime;        
        memset(&myRelativeTime, 0, sizeof(myRelativeTime));      
        myRelativeTime.tv_sec = myRelativeTime.tv_sec+interval;        
        mcwill_event_set(ev, -1, false, func, ev,evname,0);
        mcwill_timer_add(ev, &myRelativeTime);
    }
    return 0;
}

int enableTimerTask(int interval,struct mcwill_event * ev,mcwill_event_cb func,char evname[]){

   
    if(!mcwill_timer_check_exist(ev)){

        struct timeval myRelativeTime;
        memset(&myRelativeTime, 0, sizeof(myRelativeTime));
        myRelativeTime.tv_usec= myRelativeTime.tv_usec+interval; 
        mcwill_event_set(ev, -1, false, func, ev,evname,0);

        mcwill_timer_add(ev, &myRelativeTime);
    }
    return 0;
}

void mcwill_startKeepAliveCheckTask(){
    int ret;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

    if(s_tid_keepAlive!=0){       
        return;
    }

    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
	struct sched_param param;
	
	pthread_attr_setschedparam(&attr,&param);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&s_tid_keepAlive, &attr, skeepAlive_task, NULL);


	if (ret < 0) {
		
		return;
	}

};



static void *skeepAlive_task(void *param){

    char *buf;
    register int len;
    int policy=-1, priority=-1;

    int64_t cur_time= 0l;

    while(1){
       
        if(g_keepAliveinfo!=0){
            cur_time = myUptimeMillis();
            if(s_mpstatus!=MP_DISACTIVE&&cur_time!=0
                &&cur_time-g_keepAliveinfo>TIMER_DEFAULT_INTERVAR_FOR_KEEPALIVE_CHECK*1000){


               
               
                close(g_fdConnected);       
                g_fdConnected = -1;
                MPCLOG(LLV_ERROR, " watchDog(keepAlive timeout) to reset ratd  \n");
               
                toReleaseMpResource();
                
                sleep(2);             
                
                
                exit(-1);
             }
             
        }
        sleep(TIMER_DEFAULT_INTERVAR_FOR_KEEPALIVE_CHECK);//30 s

    }
    
    return NULL;
}

static void * task_mpMsgQueueDispatch(PQUEUE Q) {
    MPCLOG(LLV_ERROR,"task_mpMsgQueueDispatch >>>>>>>> Q = %p\n",Q);
    
    CreateQueue(Q,QUEUE_MAXSIZE);
    int netType = getNetTypeFromQ(Q);
    notifyMpdNetStatus(netType,NETWORK_MP_ENABLE);

   
    struct ElemType elem ;
    memset(&elem,0,sizeof(struct ElemType));

    while(!Q->quit){

        while(!Q->quit && true == EmptyQueue(Q)){ 	
            MPCLOG(LLV_INFO,"Queue(%p) is empty ... ... ...",Q);
            pthread_mutex_lock(&Q->queueMutex);            
            pthread_cond_wait(&Q->queueCond, &Q->queueMutex);          
            pthread_mutex_unlock(&Q->queueMutex);         
        }
        if(Q->quit){
            break;
        }
        
        if(true==Dequeue(Q,&elem)&&elem.justNotSend==false){

            MPCLOG(LLV_INFO,"to Dispatch Queue(%p) buffer= %p,fdIndex= %d,len = %d,release = %d\n", Q,elem.bufferAddr,elem.fdIndex,elem.len,elem.release);

            if(elem.fdIndex<3&&mpc_host.flow_name_status_key[elem.fdIndex].status.dataFd>0){
                int result = udp_write(mpc_host.flow_name_status_key[elem.fdIndex].status.dataFd,elem.bufferAddr,elem.len);

                if(result>0){

                    MPCLOG(LLV_INFO, "using [%s]/%d~[%d] ,TX buf = %p,release= %d,result = %d\r\n", 
                        mpc_host.flow_name_status_key[elem.fdIndex].netTypeName,elem.fdIndex,
                        mpc_host.flow_name_status_key[elem.fdIndex].status.dataFd,
                        elem.bufferAddr,elem.release ,result);
                }else if(result == -1){
                    echoAppException(EXCEPTION_CODE_MP_SUBFLOW_ERROR, netType);
                    
                }
            }

            if(elem.release>0){
                MPCLOG(LLV_INFO, "Queue(%p) free  buffer = %p,for release == 1\n ",Q,elem.bufferAddr);
                free(elem.bufferAddr);
            }
                
        }

    }

    int fdIndex = getFdIndexFromQ(Q);
    if(fdIndex!=-1){
        MPCLOG(LLV_ERROR, "to clear s_mp_dispatch[%d] [%p]  \n",fdIndex,s_mp_dispatch[fdIndex]);
        s_mp_dispatch[fdIndex]=0;
    }
    
    MPCLOG(LLV_ERROR,"task_mpMsgQueueDispatch  <<<<<<<<    Queue(%p) ,fdIndex = %d\n", Q,fdIndex);
    DestroyQueue(Q);

    return NULL;
}
int getNetTypeFromQ(PQUEUE Q){
    int fdIndex = getFdIndexFromQ(Q);
    int netType=fdIndex*2;
    
    assert(netType>=0);
    
    if(netType<0){
        MPCLOG(LLV_ERROR,"getNetTypeFromQ ,maybe something error\n");
        return netType;
    }else if(netType==0){
        return MP_STREAM_FLAG_MCWILL;
    }else{
        return netType;
    }    
}

int getFdIndexFromQ(PQUEUE Q){

    int fdIndex = 0;
    for(fdIndex=0;fdIndex<3;fdIndex++){
        if(Q==&Q_MP_DISPATCH[fdIndex])
            break;
    }
    if(fdIndex==3)
        return -1;
    else
        return fdIndex;

}
void finishAllDisPathTask(){

    int fdIndex = 0;
    PQUEUE Q;
    for(fdIndex=0;fdIndex<3;fdIndex++){
        Q=&Q_MP_DISPATCH[fdIndex];
        if(Q!=NULL&&s_mp_dispatch[fdIndex]!=0){
            pthread_mutex_lock(&Q->queueMutex);	
            Q->quit = true;
	        pthread_cond_broadcast(&Q->queueCond);        
	        pthread_mutex_unlock(&Q->queueMutex); 
        }
    }    
}

void finishsubflowDispathTask(__u8 numberType){

    int fdIndex = -1;
    
    if(MP_STREAM_FLAG_4G&numberType){
        fdIndex = MP_STREAM_FLAG_4G/2;
    }

    if(MP_STREAM_FLAG_MCWILL&numberType){
        fdIndex = MP_STREAM_FLAG_MCWILL/2;
    }

    if(MP_STREAM_FLAG_WIFI&numberType){
        fdIndex = MP_STREAM_FLAG_WIFI/2;
    }
    if(fdIndex!=-1&&fdIndex>=0&&fdIndex<MAX_SUBFLOWS){
        PQUEUE Q;
       
        Q=&Q_MP_DISPATCH[fdIndex];
        if(Q!=NULL&&s_mp_dispatch[fdIndex]!=0){
            pthread_mutex_lock(&Q->queueMutex);	
            Q->quit = true;
	        pthread_cond_broadcast(&Q->queueCond);        
	        pthread_mutex_unlock(&Q->queueMutex); 
        }
    }
}

void toReleaseMpResource(){
    MPCLOG(LLV_ERROR, " toReleaseMpResource...  \n");
    clearMpRouteAndIptables();
    mcwill_event_del(&s_tun_event,&mpc_host.loc_fd);
    triggerEvLoop();
    echoAppException(EXCEPTION_CODE_MP_TUN_CLOSE, MP_STREAM_FLAG_TUN0);
                

    removeAllSubFlowsEvent();
    s_mpstatus = MP_DISACTIVE;

    finishAllDisPathTask();      
   notifyMpdNetStatus(MP_STATE,NETWORK_DOWN); 
}
void updateCurrentSubFlowsNum(){
    int i,flows=0;
    for(i= 0;i<MAX_SUBFLOWS;i++){
        if(mpc_host.flow_name_status_key[i].status.sub_mp_status == NETWORK_MP_ENABLE)
          flows++  ;
    }
    mpc_host.subflow_nu=flows;    

}


void switchMPToActivite(){

    if(s_mpstatus==MP_IDLE){
        s_mpstatus = MP_ACTIVED;
        MPCLOG(LLV_ERROR,"set mp status to ACTIVE \n"); 
        echoAppException(EXCEPTION_CODE_MP_ACTIVIE,MP_STATE);
        enableAckTimer();
    }
}

unsigned long getTimeStamp(){
    struct timeval tv;
    unsigned long currentSecond = 0;
    memset(&tv, 0, sizeof(tv));
    gettimeofday(&tv, NULL);

    currentSecond = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return currentSecond;

}

unsigned long GetRTTC(int netType){

    int index  = netType/2;
    if(index<3)
        return mpc_host.flow_name_status_key[index].RTTC;
    else{
        MPCLOG(LLV_ERROR,"GetRTTC input netType(%d) ERROR\n",netType); 
        return 0;
    }
}

void notifyMpdNetStatus(int netType,int netStatus){
    if(stMpdFuncs!=NULL&&stMpdFuncs->onUpdateNetWorkStatus!=NULL){
            MPCLOG(LLV_DEBUG,"to notify MPD  onUpdateNetWorkStatus(%d ,%d).  \n",netType,netStatus);
            stMpdFuncs->onUpdateNetWorkStatus(netType,netStatus);

    }else{
        MPCLOG(LLV_ERROR,"stMpdFuncs is NULL \n");
    }
}
