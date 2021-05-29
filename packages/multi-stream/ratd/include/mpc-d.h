
#ifndef	_MPC_D_H_
#define	_MPC_D_H_
#include <stdbool.h>
//#define MPD_LIB "lib-mpd_back.so"
#define MPD_LIB "lib-mpd.so"

#define MP_OPTION_FLAG_DATA     0x01
#define MP_OPTION_FLAG_ACK      0x02
#define MP_WIRESHARK_FLAG       0x55

#define MPC_SUBFLOW_MCILL   0
#define MPC_SUBFLOW_4G      1
#define MPC_SUBFLOW_WLAN    2
#define  MAX_SUBFLOWS 3


#define MP_STATE   0x00   //MP is recreated,should to clear mp wind
#define MP_STREAM_FLAG_MCWILL   0x01
#define MP_STREAM_FLAG_4G       0x02
#define MP_STREAM_FLAG_WIFI     0x04
#define MP_STREAM_FLAG_TUN0     0x06

#define LLV_ERROR	1
#define LLV_WARNING	2
#define LLV_NOTIFY	3
#define LLV_INFO	4
#define LLV_DEBUG	5
#define LLV_DEBUG2	6

enum NETWORK_STATUS{    
    NETWORK_DOWN=0,
    NETWORK_UP,
    NETWORK_MP_DISABLE,
    NETWORK_MP_ENABLE,
};



#pragma pack(push)
#pragma pack (1)

/*
struct mp_datas_head{
    __u8 flag;
    __u8 option;
    //__u8 stream;
    //__u32 seqnumber;
    __u16 seqnumber;
    __u32 timestep;
    __u32 uid;
    __u16 reserve;
};
*/


struct mp_datas_head
{
    __u8  flag;
    __u8  length:   4;
    __u8  stream:   2;
    __u8  fragment: 2;
   
    __u16 DSN;
    __u16 SSN;
    __u16 ack;
    __u32 uid;
};

#pragma   pack(pop)



typedef const char * (*MPD_GetVersion) (void);
typedef int (*MPD_SendNewTransData) (const char *buffer, size_t datalen);
typedef int (*ParseMPData) (const char *buffer, size_t datalen);
typedef void (*MPD_SubFlowStateInd) (int netType,int status);

typedef void (*MPD_PeriodCheck) ();
typedef unsigned long (*MPD_GetRTT) (int netType);

/**
*  runtime functions from multi-path control level
*
**/
struct MPC_Env {    
    int  (* OnPostPreMpDataToTransport) (const char *buffer, size_t datalen,size_t offsize);
   // void (*getTrafficRules)         ();   
    int  (*OnSendMpDataComplete)    (int netType, char * buffer, int len,int release);   
    int  (*OnNotifyException)       (int netType,int exceptionCode);
    void (*Do_plog)                 (int prority,char *format,...);   
    void (*Free_ResendBuf)          (char *buffer,int subFlowFlag);
    unsigned long (* GetRTTC)       (int netType);
    
};

/**
*  functions offered by lib-mpd.so
*
**/
typedef struct {
    char * version;  
    MPD_SendNewTransData onSendNewTransData;    
    MPD_GetVersion       getVersion;
    ParseMPData          onParseMPData;    
    MPD_SubFlowStateInd  onUpdateNetWorkStatus;
    MPD_PeriodCheck      onPeriodCall;
    MPD_GetRTT           getRTTD;
} stMPDFunc;

#endif
