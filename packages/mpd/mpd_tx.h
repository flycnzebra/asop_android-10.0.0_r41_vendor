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
#ifndef __MPD_TX_H__
#define __MPD_TX_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "mpd.h"
/******************************************************************************/
typedef enum
{
    NO_FRAGMENT = 0,
    FIRST_FRAG_BLK,
    LAST_FRAG_BLK
}eFragment;

typedef enum
{
    SEND_FAIL = -1,
    SEND_SUCCESS
}eSendResult;

typedef struct
{
    U8* pFirstBlockAddr;
    U8* pFirstBlockDataAddr;
    U32 firstBlockLength;
    U8* pLastBlockAddr;
    U8* pLastBlockDataAddr;
    U32 lastBlockLength;
}stDataFragmentRec;

/******************************************************************************/
void MPD_TxWinInit(stUserInfo* pUser);
void MPD_txBeginInc(stUserInfo* pUser, U8 flowId);
void MPD_txNextInc(stUserInfo* pUser, U8 flowId);
U8   MPD_isSSNInTxWin(stTxWin* pTxWin, U16 ssn);
void MPD_freeTxBeginData(stUserInfo* pUser, U8 flowId);
S32  MPD_callMPCToRetrans(stUserInfo* pUser, U8 flowId, stDataNode* pDataNode);
void MPD_timeoutTxWinSuspend(stUserInfo* pUser);
//S32  MPD_SendNewTransData(U32 addressID, U8 *dataAddr, S32 length);
void MPD_clearTxWin(stUserInfo* pUser, U8 flowId);
void MPD_TxWinRetransCheck(stUserInfo* pUser);
#ifdef DATAWIN_TXBEGIN
U16 MPD_GetDataWinTxBegin(stUserInfo* pUser);
#endif
void MPD_CheckTxWinPdata(stUserInfo* pUser); //for test
U32 MPD_getTxWinUnackCnt(stUserInfo* pUser, U8 flowId);
#ifdef SCHD_NEW
bool MPD_isTxWinEmpty(stUserInfo* pUser, U8 flowId);
#endif
#ifdef SCHD_DELAY
U32 MPD_getHistAcnt(stUserInfo* pUser, U8 flowId);
#endif
#ifdef TRAFFIC_CTL
void MPD_DoSendWaitQueueData(stUserInfo* pUser);
void MPD_clearWaitQueue(stUserInfo* pUser);
#endif

#ifdef __cplusplus
};
#endif
#endif // __MPD_TX_H__
