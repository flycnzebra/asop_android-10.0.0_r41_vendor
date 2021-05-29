#ifndef	_DATAPARSE_H_
#define	_DATAPARSE_H_

#include <stdbool.h>

#define BYTE unsigned char


#define RESULT_OK       0
#define RESULT_FAILED   -1

#pragma pack(push)
#pragma pack (1)

struct r_base{
    __be32  messageType;
    __be32  sessionId;
};
struct r_app_config {    
    struct r_base baseInfo;
    __be32  uid;
    char dns[16];
    char mag[16];   
};

struct r_app_enable {    
    struct r_base baseInfo;
    __be32  netType;
    char netTypeName[13];
    __be32 band;
    __be32 rtt;
};

struct r_app_disable {    
    struct r_base baseInfo;    
};


struct r_app_add {    
    struct r_base baseInfo;
    __be32  netType;
    char netTypeName[13];
    __be32 token;
    __be32 band;
    __be32 rtt;
    char ip[16];
};


struct r_app_del {    
    struct r_base baseInfo;    
    __be32  token;
    __u8    numberType;
    __u8    deleteCause; 
};

struct r_app_enable_log{
    struct r_base baseInfo;
    __be32  operation;
};

struct r_app_mpc_keepalive {    
    struct r_base baseInfo;       
};


struct r_app_try {    
    struct r_base baseInfo;
    __be32  netType;
    char netTypeName[13];    
};



struct r_sync_mp_init {    
    struct r_base baseInfo;
    __be32  uid;    
    __u8    numberType;
};

struct r_app_release_mp {    
    struct r_base baseInfo;
    __be32 token;
    __u8   numberType;
    __u8   deleteCause; 
};



struct magRsp {    
    __be32  messageType;
    __be32  sessionId;
    __be32  len;
    BYTE result;
};
#pragma   pack(pop)


#define LEN_TYPE_ADD_SUBFLOW_REQ 25
#define LEN_TYPE_TRY_SUBFLOW_REQ (16+4+2)//4//4 for timestamp ,2 for ttl
#define LEN_TYPE_DEL_SUBFLOW_REQ 22
#define LEN_TYPE_RELEASE_MP_REQ  22

#define LEN_TYPE_SYNC_MP_INIT_REQ  17

#define LEN_TYPE_ADD_SUBFLOW_RSP 17

#define LEN_TYPE_TRY_SUBFLOW_RSP 17





static char *MESSAGE_TYPE_INFO[28]={
    "",
    "ADD_SUBFLOW_REQ",
    "ADD_SUBFLOW_RSP",
    "TRY_SUBFLOW_REQ",
    "TRY_SUBFLOW_RSP",
    "DEL_SUBFLOW_REQ",
    "DEL_SUBFLOW_RSP",
    "RELEASE_MP_REQ",
    "RELEASE_MP_RSP",//8
    "SYNC_MP_INIT_REQ",
    "SYNC_MP_INIT_RSP",//0x0a
    "",//0x0b
    "",//0x0c
    "",//0xd
    "",//0xe
    "",//0xf
    "", //0x10    
    "ENABLE_MP_REQ",//0x11 =  17
    "ENABLE_MP_RSP",
    "DISABLE_MP_REQ",
    "DISABLE_MP_RSP",
    "CONFIG_MP_REQ",
    "CONFIG_MP_RSP",
    "KEEPALIVE_REQ",
    "KEEPALIVE_RSP",
    "ENABLE_MPC_LOG",
    "EXCEPTION",
    "NOTIFY_STATICS",//27
    
    };



#define MP_SUBFLOW_NAME_WIFI                     "wlan0"
#define MP_SUBFLOW_NAME_MCWILL                   "mcwill"
#define MP_SUBFLOW_NAME_4G                       "rmnet_data0"

static char *NETTYPE_INFO[3]={
    
    MP_SUBFLOW_NAME_MCWILL,
    MP_SUBFLOW_NAME_4G,
    MP_SUBFLOW_NAME_WIFI,
    
    };

static char * GetNetType(int typeIndex){

    if(typeIndex<sizeof(NETTYPE_INFO))
        return NETTYPE_INFO[typeIndex];
    else
        return "UnDefine NetType";
}

static char * GetMessageInfo(int messageType){

    if(messageType<sizeof(MESSAGE_TYPE_INFO))
        return MESSAGE_TYPE_INFO[messageType];
    else
        return "UnDefine MessageType";
}
enum MESSAGE_TYPE{
    
    MESSAGE_TYPE_ADD_SUBFLOW_REQ=1,
    MESSAGE_TYPE_ADD_SUBFLOW_RSP,
    MESSAGE_TYPE_TRY_SUBFLOW_REQ,
    MESSAGE_TYPE_TRY_SUBFLOW_RSP,
    MESSAGE_TYPE_DEL_SUBFLOW_REQ,
    MESSAGE_TYPE_DEL_SUBFLOW_RSP,
    MESSAGE_TYPE_RELEASE_MP_REQ,
    MESSAGE_TYPE_RELEASE_MP_RSP,//8

    MESSAGE_TYPE_SYNC_MP_INIT_REQ,
    MESSAGE_TYPE_SYNC_MP_INIT_RSP,//0x0a

    MESSAGE_TYPE_ENABLE_MP_REQ=17,//0x11
    MESSAGE_TYPE_ENABLE_MP_RSP,
    MESSAGE_TYPE_DISABLE_MP_REQ,
    MESSAGE_TYPE_DISABLE_MP_RSP,
    MESSAGE_TYPE_CONFIG_MP_REQ,
    MESSAGE_TYPE_CONFIG_MP_RSP,
    MESSAGE_TYPE_KEEPALIVE_REQ,
    MESSAGE_TYPE_KEEPALIVE_RSP,
    MESSAGE_TYPE_ENABLE_MPC_LOG,
    MESSAGE_TYPE_EXCEPTION,
    MESSAGE_TYPE_NOTIFY_STATICS,

};

enum DELETE_SUBFLOW_CAUSE{
    
    DELETE_FOR_NORMAL=0,
    DELETE_FOR_EXCEPTION,
    DELETE_FOR_RELEASE,
};

void *decodeAppCommandBuffer(char * buffr,int len);

bool decodeMagCommandBuffer(struct magRsp* rsp,char * buffer,int len,short netType);


u_char *encodeAppRsp(__be32 messageType,__be32 sessionId,__be32 result);
u_char *encodeAppRspWithNetType(__be32 messageType,__be32 sessionId,__be32 netType,__be32 result);
u_char *encodeExceptionRsp(__be32 messageType,__be32 exceptioncode,__be32 netType);
void *encodeAddRequest(struct r_app_add* r_add);
void *encodeDelRequest(struct r_app_del* r_del);
void *encodeTryRequest(struct r_app_try* r_try,unsigned short sub_ut_TTl);

#endif
