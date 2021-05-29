/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_tx.h
 *
 * DESCRIPTION:
 *
 *
 * HISTORY:
 * Date        Author      Description
 * ----------  ----------  -----------------------------------------------------
 * 2018.01.04              Initial file creation.
 *
 ******************************************************************************/
#ifndef __MPD_H__
#define __MPD_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <string.h>
/******************************************************************************/
//#define FIXED_NWT // 固定调度
//#define NO_RANK //数据窗不排序测试
#define SSN_OPTIMIZE   //SSN处理优化
#define SACK_OPTIMIZE //sack处理优化
//#define SCHD_NEW    //方案最新调度算法
#define SCHD_BALANCE //多条流平衡发送窗口
#define SCHD_DELAY  //
#ifdef SCHD_BALANCE
#define SCHD_BALANCE_NEW //按照发送速率比设置发送窗口比
#define SCHD_BALANCE_NEW_p //按照发送速率比设置发送窗口比,增加丢包率的计算
#endif
#define RETRANS_OPTIMIZE//重传优化，在datawin窗口有效期内多次重传
#define CHECK_ACK
#define TRAFFIC_CTL  //流控

#ifndef MAG
#define NO_LOG //终端关闭所有log
#define MY_LOG

#endif







/******************************************************************************/
#define NUM_SUBFLOW   3  //0:mcwill 2:wifi
#define LLV_ERROR     0
#define LLV_WARNING   1
#define LLV_NOTIFY    2
#define LLV_INFO      3
#define LLV_DEBUG     4
#ifdef MAG
#define MAX_USER_NUM  100
#else
#define MAX_USER_NUM  100
#endif

#define MAX_SACK_NUM   11
#define MAX_TX_WINDOW_SIZE 1024
#define RxSubWinMaxSize         1024
#define RxDataWinMaxSize        4096
#define MaxFragNum              2
#define LEN_MPD_HEADER_WITHOUT_SACK   12
#define MPD_PACKTET_FLAG              0x55
#define MPD_IDLE_TH             1*60*100 //10分钟
#define SackTimeOut             1
#define MAX_SN                  65536 //65535 //??
#define RCV_SN_RANGE            32767   //add by caowy
#define RETRANS_TO_MC           10
#define RETRANS_TO_OTHER        6
#define T_PKT                   50
#ifdef TRAFFIC_CTL
#define T_TC_PERIOD				100  //流控监测周期
#define DataWaitQueueSize		512   //等待发送队列vbuffer大小
#endif
/******************************************************************************/
//base type define
typedef unsigned int   U32;
typedef unsigned short U16;
typedef unsigned char  U8;
typedef signed   int   S32;
typedef signed   short S16;
typedef signed   char  S8;
#ifndef __cplusplus
#define bool  char
#define true  1
#define false 0
#endif

typedef struct
{
    U8  flag;
    U8  fragment: 2;
    U8  stream:   2;
    U8  length:   4;
    U8 DSN[2];
    U8 SSN[2];
    U8 ack[2];
    U8 uid[4];
#ifdef DATAWIN_TXBEGIN    
    U8 DSNBG[2];
    U8 rev[2];
#endif
}stMPDHeader;

#ifdef CHECK_ACK
typedef struct
{
    U8  length: 4;
    U8  version: 4;
    U8  dsfield;
    U8  len[2];
    U8  id[2];
    U16 frag_offset: 13;
    U16 flags:   3;
    U8  ttl;
    U8  proto; //proto=6为TCP
    U8  checksum[2];
    U8  src[4];
    U8  dst[4];
}stIPHeader;

typedef struct
{
    U8  srcport[2];
    U8  dstport[2];
    U8  seq[4];
    U8  ack[4];
    U8 flags1:4;
    U8 length:4;
    U8 flags2;
}stTCPHeader;
#endif

typedef enum 
{
    DATA_INIT,
    WAITING_SEND,
    WAITING_ACK,
    SACKED
}eTransState;

typedef enum 
{
    DOWN,
    UP,
    MP_DISABLE,
    MP_ENABLE,
#if 0   //def MAG
	MP_NOSEND,  //MAG侧，子流处在只能收包，不能发包状态
#endif
}eSubFlowStateInd;


typedef struct 
{
    U16 DSN;                    //Data-level Sequence Number
    U16 SSN;                    //Subflow SN
    U32 magic1;
    U8* pData1;   
    U8* pData;                 //address of data
    U32 magic2;
    eTransState transState;     //transport state: waiting_ack or sacked
    U32 T1;                     //data send out time
    U32 TO_dataAck;             //ack time-out time
#ifdef RETRANS_OPTIMIZE    
    U32 TO_retrans;             //sack retrans time-out time
#endif    
    U16 reTxCnt;                //reTx times count
    U16 payloadLen;
}stDataNode;

typedef struct
{
    stDataNode* pTxBegin;       //pointer to oldest dataNode to be confirmed  //下一个待确认的节点
    stDataNode* pTxNext;        //pointer to dataNode next to the newest dataNode  //下一个来包放的位置
    stDataNode  dataNode[MAX_TX_WINDOW_SIZE];
    U16 subflowSn;
    U16 ackTimeOutcnt;          //if txBegin ack time-out count is GT SUBFLOW_SUSPEND_TH, throw subflow exception.
#ifdef SCHD_BALANCE
    U16 retransCnt;
#endif
#ifdef SCHD_DELAY
	U32 histA; //待发送数据包
	U32 lastTxTime; //最近发送时刻
#endif
    U32 lastSendAckTime;  //该流上一次发送ACK的时间
    U32 lastSendPktTime;  //该流上一次发送pkt 的时间
    //stDataNode* pTxLastRet;  //指向上一个重传包的指针
    stDataNode* pTxLast;  //发送窗中最后一个发送包
    U16 timeOutRetCnt;  //最新包超时已重传次数
    U16 lastTxBeginSsn; //保存上一次的begin SSN
}stTxWin;

typedef struct
{
    U8 status;
    U16 SSN;
}stRxSubWinNode;

typedef struct
{
#ifdef CHECK_ACK
    U8  flag;
#endif
    U8  status;
    bool timeOut;
    U8  fragment;
    U8  mpdHeadLen;
#ifdef RETRANS_OPTIMIZE    
    U8  stream;
    U16 SSN;
#endif
    U16 DSN;
    U16 msgLen[MaxFragNum];
    U32 Time;
    U8* msgPtr[MaxFragNum];
    //U8* dataPtr;
}stRxDataWinNode;

typedef struct
{
    stRxSubWinNode Win[RxSubWinMaxSize];
    U16 RxBegin;  //接收窗中第一个未收到的节点位置
    U16 RxNext;  //当前收到最大SSN节点的下一个节点位置
    U16 SentAck;//已发送的Ack
#ifdef RETRANS_OPTIMIZE
    U16 dataWinSSN;  //数据窗已发送给APP的SSN +1
#endif
    //bool NeedSendSack;
    //U16 Ack;
    //U32 lastSendPktTime;  //该流上一次发送ACK的时间
    U16 lastRxBegin;  //保存了上一次rxBegin
}stRxSubWin;

typedef struct
{
    stRxDataWinNode Win[RxDataWinMaxSize];
    U16 RxBegin;  //第一个未收到
    U16 RxNext;  //收到最后一个的下一个
#ifdef CHECK_ACK
    bool NewData;   
#endif      
}stRxDataWin;

#ifdef TRAFFIC_CTL
typedef struct
{
	U8 *PData;
	S32 len;
}stWaitSendNode;

typedef struct
{	
	U32	pktRate;     //速率,也就是收到的确认包个数
	U32 pktRatePre;  //上一个周期的发包速率
	U32	pktCntTx;	//周期发包个数
	U32	scaleFactor;  //增长系数
	U32 lossThrsh;  //丢包率门限
	U32 pktRateAvg; //滑动平均速率
	U32	throuputLimit;  //流控值
	U32	acceleratedPkt;  //速率加速度
	U32	pktLossCnt;   //周期内丢包个数
	U16	ssnBegin;
	U16 ssnEnd;
	U32 sackNum;  //期间收到几次sack消息
	U32 acceleratedCnt;  //统计发包速率加速度涨不上去的次数
	U32 quitTrafficCtlCnt;
	bool isTrafficCtl;  //是否在流控状态
	bool isPktTxStop;   //该流是否停止发包
}stTrafficControl;
#endif

typedef struct
{
    U8  isValid;
    U32 RTTNew;
    U32 RTTFlow;
    U32 Nweight;
    U32 wCnt;
#ifdef SCHD_NEW
    U32 RTTDev;
    U32 RTO;
#endif
#ifdef SCHD_DELAY
	U32 InhertDelay;  //子流系统时延,单位ms
#endif
#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
    U32 PktAvg;
    U32 PktCnt;  //期间收到确认的发送包个数
	U32 pktRetCnt;  //cwy add
	U32 PktChooseCnt;    
    U32 ModifyFactor;
	
#if	1
	U32 txWinEmptyCnt;	
#endif
    U8  ThCnt;
    bool isSendPkt;  //该流是否被调度发包
#endif
#if 0
	bool isSchedValid;//只收包，不发包状态，该流不能调度
#endif
#ifdef SCHD_DELAY
	bool isRttUpdate;
#endif
    bool isPeriodRttUpdate;  //是否周期更新该流的RTT
}stSchdStreamPara;

typedef struct  
{
    U8 leftEdgeSSN[2];
    U8 rightEdgeSSN[2];
}stSACKBlock;

typedef struct _StUserInfo
{
    U32                 addressID;                 
    U32                 uid;
    U32                 lastPackageTime;  //最近一个收发包时间 + 1分钟
    //tx window
    stTxWin             txWindow[NUM_SUBFLOW];
    U16                 txDataSn;
    U16                 rxDsnErr;
    //rx window
    stRxSubWin          RxSubWin[NUM_SUBFLOW + 1];
    stRxDataWin         RxDataWin;
    U32                 SendSackTime;
    //schdule
    stSchdStreamPara    schdPara[NUM_SUBFLOW];
    S32                 RTTFlowMax;
#ifdef TRAFFIC_CTL
	//traffic control
	U32					pDataWaiteHead;  //下一个MPC来的待发包放的位置
	U32					pDataWaiteTail;	//下一个调度发送的包从这取
	stWaitSendNode		dataWaitQueue[DataWaitQueueSize];  //等待调度的数据放在一个队列中
	stTrafficControl    trafficControl[NUM_SUBFLOW];
	U32 trafficCtlTime;  //记录周期流控判断时间点
	U32 throuputTotal;   //每秒钟各流总共可发送数据包数量
	bool isAllFlowTrafficCtl;  //是否所有的子流都在流控状态，如果都在流控状态，则周期从等待队列取包个数受限制
#endif
    //sack
    stSACKBlock         sackBlock[MAX_SACK_NUM];
#ifdef TRAFFIC_CTL
	U32  tcDelPktCnt;  //流控扔包个数
	U32  txPktCnt;  //Mp下发送包个数计数
	U32  txPktFreeCnt; 
	bool isMpStopSched;   //本周期多流停止调度
#endif
    U32  totalToAckNum;  //记录超时ACK连续发送总个数
    U32  activeFlowNum;  //使能的流个数
    S32 exceptionCode[3]; //wangweiTest
}stUserInfo;

#ifndef MAG
typedef struct
{
	char* version;
	S32  (*MPD_SendNewTransData)(U8 *dataAddr, S32 length);
	char*(*MPD_GetVersion)();
	S32  (*ParseMPData) (const char* buffer,U32 datalen);
	void (*MPD_SubFlowStateInd)(U32 flowId, U32 state);
	void (*MPD_periodCheck)();
    unsigned long (*MPD_GetRTT)(int netType);
}stMPDFunc;

typedef struct 
{
    int  (*OnPostPreMPDataToTransport)(const char * buffer, size_t datalen, size_t offsize);
    int  (*OnSendMPDataComplete)(int fd, const char * buffer, size_t datalen, int release);
    int  (*OnNotifyException)(int netType, int exceptionCode);
    void (*do_plog)(int priority, char *format, ...);
    void (*Free_ResendBuf)(char * buffer,int subFlowFlag);
    unsigned long (*GetRTTC)(int netType);
}RAT_Env;
#endif
/******************************************************************************/
extern U32 g_time;
//typedef struct _StUserInfo stUserInfo;
/******************************************************************************/
#ifndef MAG
stMPDFunc* MPD_Init(U32 uid, RAT_Env* funPtr);
#endif
void MPD_log(U32 uid, U8 logLevel, const char* format, ...);
void MPD_setPrintLvl(U8 logLevel);
S32  MPD_GetDeviceId(U32 flowId);
int MPD_PostPreMPDataToTransport (unsigned int addressID, const char * buffer, size_t datalen, size_t offsize);
int  MPD_OnSendMPDataComplete(unsigned int addressID, int fd, const char * buffer, size_t datalen, int release);
int  MPD_OnNotifyException(unsigned int addressID, int netType, int exceptionCode);
U8* MPD_memAlloc(size_t size);
void MPD_memFree(void *memblock);
U8* MPD_MsgMemCreat(size_t size);
void MPD_MsgMemfree(void *memblock);
U8* MPD_GetMsgDataPtr(void *memblock ,bool bOffset);
void MPD_RetransMsgMemfree(void *memblock, int flowId);
#ifndef MAG
void MPD_CheckNoTraffic(stUserInfo* pUser);  //add  0821
#endif
/******************************************************************************/
#ifdef WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\')?(strrchr(__FILE__, '\\')+1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/')?(strrchr(__FILE__, '/')+1):__FILE__)
#endif
#ifdef NO_LOG
#define MPDLOG(uid, logLevel, pFormat, ...)   
#else
#define MPDLOG(uid, logLevel, pFormat, ...)  MPD_log(uid, logLevel, "MPD[%010u][%010u][%s]line %5d: "pFormat" \n", g_time, uid, __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif


#ifdef MY_LOG
#define MYLOGP(uid, logLevel, pFormat, ...) MPD_log(uid, logLevel, "MPD[%010u][%010u][%s]line %5d: "pFormat" \n", g_time, uid, __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define MYLOGP(uid, logLevel, pFormat, ...)
#endif

#define MYLOG(uid, logLevel, pFormat, ...)

#define DISTANCESN(a,b)     (((b)+MAX_SN-(a)) & (MAX_SN-1))  //cwy modify 0912
#ifdef __cplusplus
};
#endif
#endif // __MPD_H__
