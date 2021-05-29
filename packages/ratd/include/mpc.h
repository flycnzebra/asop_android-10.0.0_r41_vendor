
#ifndef	_MPC_H_
#define	_MPC_H_
#include <stdbool.h>
#define MPD_LIB "lib-mpd.so"
#include "../mcwill_event.h"
#include "mpc-d.h"
#include <sys/sysinfo.h>


#define MPC_VERSION "1.0.2.18.6(5075/5086)-h"

#define TIMER_DEFAULT_INTERVAR_FOR_SACK 1000*10  
#define TIMER_DEFAULT_INTERVAR_FOR_KEEPALIVE_CHECK 30  //S  

#define TIMER_DEFAULT_INTERVAR_FOR_STATICS  5
#define TIMER_DEFAULT_INTERVAR_FOR_WATCHDOG 10
#define MP_DATA_PORT    5075
#define MP_COMMAND_PORT 5086


#define EXCEPTION_CODE_MP_IDLE          -6
#define EXCEPTION_CODE_MP_ACTIVIE       -5
#define EXCEPTION_CODE_MP_TUN_CLOSE     -3
#define EXCEPTION_CODE_MP_SUBFLOW_ERROR -1


#define ONE_M 1*1024*1024


#define MP_SUBFLOW_NAME_WIFI                     "wlan0"
#define MP_SUBFLOW_NAME_MCWILL                   "mcwill"
#define MP_SUBFLOW_NAME_4G                       "rmnet_data0"



#define NODE_MESSGETYPE     "\"messageType\""
#define RE_NODE_MESSGETYP   "\"messageType\":%d"    
#define NODE_SESSION_ID     "\"sessionid\""
#define RE_NODE_SESSION_ID  "\"sessionid\":%d"  

#define NODE_NET_TYPE       "\"netType\""
#define RE_NODE_NET_TYPE    "\"netType\":%d"

#define NODE_TOKEN          "\"token\""
#define RE_NODE_TOKEN       "\"token\":%d"\


#define NODE_SUBFLOW_IP     "\"ip\""
#define RE_NODE_SUBFLOW_IP  "%*[\"a-zA-Z: ]%[0-9.][^a-z\"}, ]"

#define NODE_BAND           "\"band\""
#define RE_NODE_BAND        "\"band\":%d"

#define NODE_RTT            "\"rtt\""
#define RE_NODE_RTT         "\"rtt\":%d"

#define NODE_NET_TYPE_NAME      "\"netTypeName\""
#define RE_NODE_NET_TYPE_NAME   "\"netTypeName\":%[^\",]"

#define RE_NODE_NET_TYPE_NAME_WITH_QUOTATION_MARKS   "\"netTypeName\":%*[\"]%[^\",]"

#define NODE_NUMBER_TYPE    "\"numberType\""
#define RE_NODE_NUMBER_TYPE "\"numberType\":%d"
#define NODE_DELETE_CAUSE   "\"deleteCause\""
#define RE_NODE_DELETE_CAUSE     "\"deleteCause\":%d"

#define NODE_OPERATION      "\"operation\""        

#define RE_NODE_OPERATION   "\"operation\":%d"

#define NODE_UID            "\"uid\""
#define RE_NODE_UID         "\"uid\":%d"
#define NODE_DNS            "\"dns\""
#define RE_NODE_DNS         "%*[\"a-zA-Z: ]%[0-9.][^a-z\"}, ]"
#define NODE_MAG            "\"mag\""
#define RE_NODE_MAG         "%*[\"a-zA-Z: ]%[0-9.][^a-z\"}, ]"


#define NODE_LOCAL_IP            "\"localip\""
#define RE_NODE_LOCAL_IP         "%*[\"a-zA-Z: ]%[0-9.][^a-z\"}, ]"

struct netStatus {
    int sub_mp_status;
    int commandFd;
    int dataFd;

    unsigned long long haveEchoedstatistics;
    unsigned long long statistics;
    struct mcwill_event data_event;
    struct mcwill_event command_event;
};
struct subFlowMap {
	int key;
	char *netTypeName;
    unsigned long RTTC;
    struct netStatus status;
	char *(*f) __P((int));
};

struct MPC_OPTS {
    char *table_name;
    char *dns_addr;
    char *svr_addr;    
    int  svr_data_port;   
    int  svr_command_port;    
    int uid;
    int  quiet;      
    
    char *localIp; 
    
};

struct MPC_HOST {
   char *host;   
   char *dev;
   int  loc_fd;
   int subflow_nu;
   
   struct subFlowMap flow_name_status_key[MAX_SUBFLOWS];
   struct sysinfo detectTimeInfo[MAX_SUBFLOWS];

};


static struct subFlowMap _flow_name_status_key[] = {
    { MPC_SUBFLOW_MCILL,	MP_SUBFLOW_NAME_MCWILL,0L,  {false,-1,-1,0,0,{NULL,NULL,-1,-1,false},{NULL,NULL,-1,-1,false}},  NULL},
    { MPC_SUBFLOW_4G,	    MP_SUBFLOW_NAME_4G,    0L,  {false,-1,-1,0,0,{NULL,NULL,-1,-1,false},{NULL,NULL,-1,-1,false}},  NULL},
    { MPC_SUBFLOW_WLAN,	MP_SUBFLOW_NAME_WIFI,  0L,  {false,-1,-1,0,0,{NULL,NULL,-1,-1,false},{NULL,NULL,-1,-1,false}},  NULL},
};


#define MPC_DEV_LEN 20



#define MPC_MAX_MP_HEAD_LEN     12  
#define MPC_FRAME_SIZE          1500
#define MPC_FRAME_OVERHEAD      100
#define MPC_FSIZE_MASK          0x0fff

#define MPC_CONN_CLOSE 0x1000
#define MPC_ECHO_REQ   0x2000
#define MPC_ECHO_REP   0x4000
#define MPC_BAD_FRAME  0x8000


extern struct MPC_OPTS mpc_op;
extern struct MPC_HOST mpc_host;

#define MAX_COMMAND_BYTES (4 * 1024)

#define HAVE_STRLCPY 1
#define ANDROID_SOCKET_ENV_PREFIX	"ANDROID_SOCKET_"

struct BufInfo {    
    char * buffer;
    int bufferSize;
};

enum MP_STATUS{
    MP_ACTIVED,
    MP_IDLE,
    MP_DISACTIVE
};

#endif
