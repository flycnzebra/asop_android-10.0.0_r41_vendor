/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_rx.h
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

#include "mpd.h"
/******************************************************************************/
#define DataWinWaitTimeOut      30 //数据排序窗超时
#define SackTag                 0x01
#ifdef MAG
#define IPUDPHeadOffset           0//28  //mag包含IP和UDP头 
#else
#define IPUDPHeadOffset           0   
#endif
//#define MPDataOffset            IPUDPHeadOffset+sizeof(stMPDHeader) 
#define MaxDSN                  65535
#define MaxSSN                  65535
/******************************************************************************/

typedef struct
{
    U8 stream;
    U8 fragment;//从bit0开始，每一位代表一个分片
    U16 DSN;
    U16 SSN;
    U16 msgLen;
    U8 mpdHeadLen;
    U8* msgPtr;
}stMPDNodeInfo;

typedef struct
{
    U8 SackL[2];
    U8 SackR[2];
}stSackV;

typedef struct
{
    U8 Tag;
    U8 Len;
	U8 Rsv[2];
    stSackV Val;
}stSack;

typedef enum
{
    UnReceived,
    Received,
    ReceiveComplete
}eNodeStatus;

typedef enum
{
    NoFrag,
    FirstFrag,
    SecondFrag,
    AllFrag = (1<<MaxFragNum)-1,
}eFrag;

typedef enum
{
    UnChecked,
    Checked,
    Sended,
}eAckFlag;

/******************************************************************************/

