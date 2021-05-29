/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_tx.c
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
#include "mpd_tx.h"
#include "mpd_schd.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************/
#define LEN_DATA_TH                   1480 //1500 Bytes - 20 Bytes IPHeader
#define LEN_FIRST_DATA_FRAG           1000
#define ACK_TIMEOUT_THD               500  // 3000 //unit 10ms
#define DATA_SEND_OVERFLOW_THD        800
/******************************************************************************/
extern U32 g_time;
extern U16 MPD_GetSubFlowAck(stUserInfo* pUser, U8 index);
#ifndef MAG
extern RAT_Env g_mpcFun;
#endif

extern int g_isTrafficCtl; //0,off, 1,on


/******************************************************************************/

void MPD_TxWinInit(stUserInfo* pUser)
{
    int sfId; //subflow id

    MPDLOG(pUser->uid, LLV_WARNING,"MPD_TxWinInit and set txDataSn   0");
    pUser->txDataSn = 0;
    memset(pUser->txWindow, 0, sizeof(stTxWin)*NUM_SUBFLOW);

    for(sfId = 0; sfId<NUM_SUBFLOW; sfId++)
    {
        pUser->txWindow[sfId].pTxBegin = &(pUser->txWindow[sfId].dataNode[0]);
        pUser->txWindow[sfId].pTxNext  = &(pUser->txWindow[sfId].dataNode[0]);
    }
}

void MPD_txBeginInc(stUserInfo* pUser, U8 flowId)
{
    stTxWin* pTxWin = &(pUser->txWindow[flowId]);
    //reset dataNode state
    memset(pTxWin->pTxBegin, 0, sizeof(stDataNode));
    //move txBegin
    if(pTxWin->pTxBegin ==  &pTxWin->dataNode[MAX_TX_WINDOW_SIZE-1])
    {
        pTxWin->pTxBegin = &pTxWin->dataNode[0];
    }
    else
    {
        pTxWin->pTxBegin++;
    }
    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_txBeginInc() ptxBegin-->DataNode[%d]", (pTxWin->pTxBegin - (&pTxWin->dataNode[0])));
}

void MPD_freeTxBeginData(stUserInfo* pUser, U8 flowId)
{
    stTxWin* pTxWin = &(pUser->txWindow[flowId]);

    if(pTxWin->pTxBegin->transState == DATA_INIT)
    {
        MPDLOG(pUser->uid, LLV_INFO, "MPD_freeTxBeginData(), but data transState %d ssn %d", pTxWin->pTxBegin->transState, pTxWin->pTxBegin->SSN);
        return;
    }
    if(NULL == pTxWin->pTxBegin->pData)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_freeTxBeginData(), but data is NULL");
        return;
    }
    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_freeTxBeginData() sfId %d nodeSSN %d", flowId, pTxWin->pTxBegin->SSN);

    //MYLOG(0xffff, LLV_WARNING,"MPD_freeTxBeginData, MPD_MsgMemfree, pTxBegin->transState %d !", pTxWin->pTxBegin->transState);
        //{
    if((pTxWin->pTxBegin->magic1 != 0x5a5a5a5a) || (pTxWin->pTxBegin->magic2 != 0x5a5a5a5a))
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_freeTxBeginData() magicError sfId %d nodeSSN %d magic1 0x%x magic2 0x%x", 
            flowId, pTxWin->pTxBegin->SSN, pTxWin->pTxBegin->magic1, pTxWin->pTxBegin->magic2);
    }

        //}
    if(pTxWin->pTxBegin->transState == SACKED)
    {
        //MYLOG(0xffff, LLV_WARNING,"MPD_freeTxBeginData, Free_ResendBuf, flowId %d, ssn %d !", flowId, pTxWin->pTxBegin->SSN);
#if 0  //def MAG
MPDLOG(pUser->uid, LLV_ERROR, "MPD_RetransMsgMemfree(), flowId %d,data is 0x%x", flowId, pTxWin->pTxBegin->pData);
#endif
        MPD_RetransMsgMemfree(pTxWin->pTxBegin->pData, flowId);
    }
    else
    {
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", pTxWin->pTxBegin->pData);
#if 0  //def MAG
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_MsgMemfree(), flowId %d,data is 0x%x", flowId, pTxWin->pTxBegin->pData);
#endif
        MPD_MsgMemfree(pTxWin->pTxBegin->pData);
    }
    pTxWin->pTxBegin->pData = NULL;

    pTxWin->pTxBegin->pData1 = NULL;
}

void MPD_txNextInc(stUserInfo* pUser, U8 flowId)
{
    stTxWin* pTxWin = &pUser->txWindow[flowId];
    //U16 ssn = pTxWin->pTxNext->SSN;

    if(pTxWin->pTxNext == &pTxWin->dataNode[MAX_TX_WINDOW_SIZE-1])
    {
        pTxWin->pTxNext = &pTxWin->dataNode[0];
    }
    else
    {
        pTxWin->pTxNext++;
    }
    pTxWin->pTxNext->SSN = pTxWin->subflowSn;  //test
    
    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_txNextInc() ptxNext->DataNode[%d], txbegin %d, txnext %d", 
		pTxWin->pTxNext - (&pTxWin->dataNode[0]), pTxWin->pTxBegin->SSN, pTxWin->pTxNext->SSN);

    if(pTxWin->pTxNext == pTxWin->pTxBegin)
    {
        MPDLOG(pUser->uid, LLV_WARNING, "Warning, MPD_txNextInc() pTxWin->pTxNext == pTxWin->pTxBegin beginSsn %d ", pTxWin->pTxBegin->SSN);
        MPD_freeTxBeginData(pUser, flowId);
        MPD_txBeginInc(pUser, flowId); //force move txBegin because tx window is full
    }

    pTxWin->pTxNext->SSN = pTxWin->subflowSn;  //test
}

U8 MPD_isSSNInTxWin(stTxWin* pTxWin, U16 ssn)
{
    if (DISTANCESN(pTxWin->pTxBegin->SSN, ssn) <= DISTANCESN(pTxWin->pTxBegin->SSN ,(pTxWin->pTxNext-1)->SSN))
    {
        return 1;
    }
    return 0;
}
S32 MPD_SplitDataMem(stUserInfo* pUser, U8* dataAddr, S32 length, stDataFragmentRec* pDataFrag)
{
    U8 *pAllocMem = NULL;
    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_SplitDataMem(), length: %d", length);
    if(length < LEN_FIRST_DATA_FRAG)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_SplitDataMem() needn't split, length %d", length);
        return -1;
    }

    //alloc first block memory 
    pAllocMem = (U8*)MPD_MsgMemCreat(LEN_FIRST_DATA_FRAG + LEN_MPD_HEADER_WITHOUT_SACK);
    MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemCreat2 address 0x%x ", pAllocMem);
    if(NULL == pAllocMem)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_SplitDataMem(), alloc first block memory failed.");
        return -2;
    }
    pDataFrag->pFirstBlockAddr = pAllocMem;
    pDataFrag->pFirstBlockDataAddr = MPD_GetMsgDataPtr(pAllocMem, false);
    pDataFrag->firstBlockLength = LEN_FIRST_DATA_FRAG + LEN_MPD_HEADER_WITHOUT_SACK;
    memcpy(pDataFrag->pFirstBlockDataAddr, MPD_GetMsgDataPtr(dataAddr, false),pDataFrag->firstBlockLength);

    //alloc last block memory
    pAllocMem = (U8*)MPD_MsgMemCreat(length - pDataFrag->firstBlockLength + LEN_MPD_HEADER_WITHOUT_SACK);
    MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemCreat3 address 0x%x ", pAllocMem);
    if (NULL == pAllocMem)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_SplitDataMem(), alloc second block memory failed.");
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", pDataFrag->pFirstBlockAddr);
        MPD_MsgMemfree(pDataFrag->pFirstBlockAddr);
        return -3;
    }
    pDataFrag->pLastBlockAddr  = pAllocMem;
    pDataFrag->pLastBlockDataAddr = MPD_GetMsgDataPtr(pAllocMem, false);
    pDataFrag->lastBlockLength = length - pDataFrag->firstBlockLength + LEN_MPD_HEADER_WITHOUT_SACK;
    memcpy((pDataFrag->pLastBlockDataAddr + LEN_MPD_HEADER_WITHOUT_SACK), 
           (MPD_GetMsgDataPtr(dataAddr, false) + pDataFrag->firstBlockLength), 
           length - pDataFrag->firstBlockLength); //memcpy data must skip MPD header

    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_SplitDataMem():firstBlockAddr 0x08%X length %d; lastBlockDataAddr 0x%08X length %d",
        pDataFrag->pFirstBlockDataAddr, pDataFrag->firstBlockLength,
        pDataFrag->pLastBlockDataAddr, pDataFrag->lastBlockLength);

    MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", dataAddr);
    MPD_MsgMemfree(dataAddr);
    
    return 0;
}

void MPD_fillMPDHeader(stUserInfo* pUser, U8 flowId, U8 *dataAddr, U8 frag)
{
    stMPDHeader *pMPDHeader = (stMPDHeader*)dataAddr;
    stTxWin     *pTxWin     = &(pUser->txWindow[flowId]);
    U16 ack = MPD_GetSubFlowAck(pUser, flowId); 
#ifdef DATAWIN_TXBEGIN     
    U16 dataTxBegin = MPD_GetDataWinTxBegin(pUser);
#endif

    pMPDHeader->flag     = MPD_PACKTET_FLAG;
    pMPDHeader->length   = LEN_MPD_HEADER_WITHOUT_SACK/4;
    pMPDHeader->stream   = flowId;
    pMPDHeader->fragment = frag;
    pMPDHeader->DSN[0]      = pUser->txDataSn>>8;
    pMPDHeader->DSN[1]      = pUser->txDataSn;
    pMPDHeader->SSN[0]      = pTxWin->subflowSn>>8;
    pMPDHeader->SSN[1]      = pTxWin->subflowSn;
    pMPDHeader->ack[0]      = ack>>8; 
    pMPDHeader->ack[1]      = ack; 
    pMPDHeader->uid[0]      = pUser->uid>>24;
    pMPDHeader->uid[1]      = pUser->uid>>16;
    pMPDHeader->uid[2]      = pUser->uid>>8;
    pMPDHeader->uid[3]      = pUser->uid;
    pUser->txDataSn++;
    pTxWin->subflowSn++;
}

stDataNode* MPD_putDataToTxWin(stUserInfo* pUser, U8 flowId, U8 *dataAddr, S32 dataLen)
{
    stMPDHeader* pHeader = (stMPDHeader*)MPD_GetMsgDataPtr(dataAddr, false);
    stTxWin*     pTxWin  = &pUser->txWindow[flowId];
    U16          SSN     = (pHeader->SSN[0]<<8)+pHeader->SSN[1];
    stDataNode*  pNode   = &(pTxWin->dataNode[SSN%MAX_TX_WINDOW_SIZE]);

    pNode->DSN        = (pHeader->DSN[0]<<8)+pHeader->DSN[1];
    pNode->SSN        = SSN;
    pNode->pData      = dataAddr;
    pNode->pData1      = dataAddr;
    pNode->transState = WAITING_SEND;
    pNode->T1         = g_time;
    pNode->reTxCnt    = 0;
    pNode->TO_dataAck = g_time + ACK_TIMEOUT_THD;
    pNode->magic1 = 0x5a5a5a5a;
    pNode->magic2 = 0x5a5a5a5a;
    
#ifdef RETRANS_OPTIMIZE
    pNode->TO_retrans = g_time + 100;  //temp
#endif
    pNode->payloadLen = dataLen;

    pTxWin->pTxLast = pNode;  //保存最后一个发送包指针
    pTxWin->timeOutRetCnt = 0;
    MPDLOG(pUser->uid, LLV_WARNING, "MPD_putDataToTxWin(): flowId %d dataNode[%d] DSN %d SSN %d pData 0x%08X T1 %d payloadLen %d. txWindow begin %d",
        flowId, (SSN%MAX_TX_WINDOW_SIZE), pNode->DSN, pNode->SSN, pNode->pData, pNode->T1, pNode->payloadLen, pTxWin->pTxBegin->SSN);

    MPD_txNextInc(pUser, flowId);
    pTxWin->lastSendPktTime = g_time;

#ifdef SCHD_DELAY
	if(pTxWin->lastTxTime == 0)
	{
		pTxWin->histA = 1;
	}
	else
	{
	#if 0//调度之前计算?
		U32 txCnt = (g_time - pTxWin->lastTxTime)*pUser->schdPara[flowId].PktAvg/100;
		if(pTxWin->histA > txCnt)
		{
			pTxWin->histA -= txCnt;
		}
		else
		{
			pTxWin->histA = 0;
		}
#endif
		pTxWin->histA++;
	}
#endif

	
    return pNode;
}

S32 MPD_callMPCToSend(stUserInfo* pUser, U8 flowId, stDataNode* pDataNode)
{
    S32 result = SEND_FAIL;
    MPDLOG(pUser->uid, LLV_INFO, "MPD_callMPCToSend(), flowId %d dataSSN %d", flowId, pDataNode->SSN);
    result = MPD_OnSendMPDataComplete(pUser->addressID, MPD_GetDeviceId(flowId), (const char*)pDataNode->pData, pDataNode->payloadLen, false);

#ifdef MAG
    //for test
    if(pDataNode->pData != pDataNode->pData1)
    {
        MPDLOG(pUser->uid, LLV_WARNING, "MPD_callMPCToSend(), flowId %d dataSSN %d, pData 0x%x, pData1 0x%x", flowId, pDataNode->SSN, pDataNode->pData, pDataNode->pData1);
    }
#endif

    if(result == SEND_FAIL)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_callMPCToSend(), flowId %d dataSSN %d, call MPC to send fail", flowId, pDataNode->SSN);
        return result;
    }
    pUser->txWindow[flowId].ackTimeOutcnt++;
    pDataNode->transState = WAITING_ACK;
    return SEND_SUCCESS;
}

S32 MPD_callMPCToRetrans(stUserInfo* pUser, U8 flowId, stDataNode* pDataNode)
{
    S32 result = SEND_FAIL;
    stTxWin* pTxWin = &(pUser->txWindow[flowId]);
    
#ifndef RETRANS_OPTIMIZE
    if (pTxWin->pTxBegin != pDataNode)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_callMPCToRetrans(), error, flowId %d dataNode not txBegin!", flowId);
        return SEND_FAIL;
    }
#endif

    MPDLOG(pUser->uid, LLV_DEBUG, "MPD_callMPCToRetrans(), flowId %d dataSSN %d", flowId, pDataNode->SSN);
#ifdef RETRANS_OPTIMIZE
    result = MPD_OnSendMPDataComplete(pUser->addressID, MPD_GetDeviceId(flowId), (const char*)pDataNode->pData, pDataNode->payloadLen, false);
#else
    result = MPD_OnSendMPDataComplete(pUser->addressID, MPD_GetDeviceId(flowId), (const char*)pDataNode->pData, pDataNode->payloadLen, true);
#endif

    //重传包也计算待发送包数
#ifdef SCHD_DELAY
    pTxWin->histA++;
#endif
    if (result == SEND_FAIL)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_callMPCToRetrans(), flowId %d dataSSN %d, call MPC to reTx fail", flowId, pDataNode->SSN);
    }
    return result;
}

S32 MPD_DoSendNewTransData(stUserInfo* pUser, U8 *dataAddr, S32 length)
{
    S32  result = SEND_FAIL;
    U8   flowId = INVALID_STREAM;
    bool isFragment = false;
    stDataFragmentRec dataFrag;
    stDataNode* pDataNode = NULL;
    U32 uid = pUser->uid;
#ifndef MAG    
    MPDLOG(uid, LLV_DEBUG, "MPD_DoSendNewTransData");
#endif

///////////////////speed test
    //MPD_OnSendMPDataComplete(pUser->addressID, 2, dataAddr, length, true);
    //return SEND_SUCCESS;
///////////////////
	
	pUser->txPktCnt++;
    if (length < LEN_MPD_HEADER_WITHOUT_SACK)
    {
        MPDLOG(uid, LLV_ERROR, "memroy length error, send fail.");
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", dataAddr);
        MPD_MsgMemfree(dataAddr);
        return SEND_FAIL;
    }	

	//????????????,?????,???????????
#ifdef TRAFFIC_CTL
	if(g_isTrafficCtl && ((pUser->pDataWaiteHead != pUser->pDataWaiteTail) || (pUser->isMpStopSched)))	//??????????
	{
	//等待发送队列非空，或多流停止调度状态时候
		pUser->dataWaitQueue[pUser->pDataWaiteHead].PData = dataAddr;
		pUser->dataWaitQueue[pUser->pDataWaiteHead].len = length;
		pUser->pDataWaiteHead = (pUser->pDataWaiteHead + 1)%DataWaitQueueSize;
		if(pUser->pDataWaiteHead == pUser->pDataWaiteTail)
		{
			//delete the tail node
			pUser->tcDelPktCnt++;
			MPDLOG(uid, LLV_ERROR, "MPD_SCHD_ChooseStream delete the tail node %d tcDelPktCnt %d", pUser->pDataWaiteHead, pUser->tcDelPktCnt);
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", pUser->dataWaitQueue[pUser->pDataWaiteTail].PData);
            MPD_MsgMemfree(pUser->dataWaitQueue[pUser->pDataWaiteTail].PData);	
			pUser->dataWaitQueue[pUser->pDataWaiteTail].PData = NULL;
			pUser->pDataWaiteTail = (pUser->pDataWaiteTail + 1)%DataWaitQueueSize;
		}

		return 1;  //put to the wait list
	}
#endif

    //schd subflow 
    flowId = MPD_SCHD_ChooseStream(pUser);
    if (flowId == INVALID_STREAM)
    {
        MPDLOG(uid, LLV_ERROR, "MPD_SCHD_ChooseStream fail dataAddr 0x%x.", dataAddr);
        MYLOG(0xffff, LLV_WARNING,"MPD_DoSendNewTransData error: chooseStream failed,isValid %d, %d, %d!", 
            pUser->schdPara[0].isValid, pUser->schdPara[1].isValid, pUser->schdPara[2].isValid);
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", dataAddr);
        MPD_MsgMemfree(dataAddr);
        return -2;   //SEND_FAIL;
    }
    pUser->lastPackageTime = g_time + MPD_IDLE_TH;
    //check data length
    if(length >= LEN_DATA_TH)
    {
        S32 rst = MPD_SplitDataMem(pUser, dataAddr, length, &dataFrag);
        if(rst == 0)
        {
            isFragment = true;
        }
    }
    MPDLOG(uid, LLV_DEBUG, "MPD_SendNewTransData(), schd flow %d isFragment %d", flowId, isFragment);
    //fill mpd header
    //put data to tx window
    //send data    
    if (!isFragment)
    {
        MPD_fillMPDHeader(pUser, flowId, MPD_GetMsgDataPtr(dataAddr, false), NO_FRAGMENT);
        pDataNode = MPD_putDataToTxWin(pUser, flowId, dataAddr,length);
        result = MPD_callMPCToSend(pUser, flowId, pDataNode);
    }
    else
    {
        //first frag
        MPD_fillMPDHeader(pUser, flowId, dataFrag.pFirstBlockDataAddr, FIRST_FRAG_BLK);
        pDataNode = MPD_putDataToTxWin(pUser, flowId, dataFrag.pFirstBlockAddr, dataFrag.firstBlockLength);
        result = MPD_callMPCToSend(pUser, flowId, pDataNode);
        //last frag
        MPDLOG(pUser->uid, LLV_WARNING,"MPD_DoSendNewTransData and set txDataSn--   %d", pUser->txDataSn);
        pUser->txDataSn--; //frags use the same DSN
        MPD_fillMPDHeader(pUser, flowId, dataFrag.pLastBlockDataAddr, LAST_FRAG_BLK);
        pDataNode = MPD_putDataToTxWin(pUser, flowId, dataFrag.pLastBlockAddr, dataFrag.lastBlockLength);
        result = MPD_callMPCToSend(pUser, flowId, pDataNode);
    }
    if (result == SEND_FAIL)
    {
        MYLOG(0xffff, LLV_WARNING,"MPD_DoSendNewTransData error: MPD_callMPCToSend failed,stream %d!", flowId);
        return -3;   //  SEND_FAIL;
    }

#if 0 //def TRAFFIC_CTL
	if(pUser->isMpStopSched)
	{
		U16 time = T_TC_PERIOD + pUser->trafficCtlTime - g_time;
		return time;  //todo
	}
#endif

    return SEND_SUCCESS;
}

void MPD_timeoutTxWinSuspend(stUserInfo* pUser)
{
    //return; //mpd不检测长时间没包释放会话
    
    int flowId;
    stTxWin* pTxWin;
    U8 i;
    U8 flag = 0;
    S32 exceptionCode = -2;
    for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
    {
        pTxWin = &pUser->txWindow[flowId];
        if (!MPD_SCHD_getFlowValid(pUser, flowId))
            continue;
        if(((pTxWin->pTxBegin->transState != DATA_INIT) && (pTxWin->pTxBegin->TO_dataAck == g_time))
            //|| (pUser->lastPackageTime == g_time)
            //|| (pTxWin->ackTimeOutcnt >= DATA_SEND_OVERFLOW_THD)
        )
        {
            MPDLOG(pUser->uid, LLV_WARNING, "MPD_timeoutTxWinSuspend() flowId %d, ackTimeOutcnt %d, TO_dataAck %d, lastPackageTime %d, g_time %d", flowId, pTxWin->ackTimeOutcnt, pTxWin->pTxBegin->TO_dataAck, pUser->lastPackageTime, g_time);
#if 0
            MPD_SCHD_setFlowValid(pUser, flowId, false);
            MPD_clearTxWin(pUser, flowId);
            MPD_ClearSubWin(pUser, flowId);
            for(i=0;i<NUM_SUBFLOW;i++)
            {
                flag |= MPD_SCHD_getFlowValid(pUser, i);
            }
            if(flag == 0)
            {
                MPDLOG(pUser->uid, LLV_NOTIFY,"Clear DataWin");
                MPD_ClearDataWin(pUser);
                pUser->txDataSn = 0;
            }
            if(pUser->lastPackageTime == g_time)
            {
                exceptionCode = -4;//idle
            }
#endif
            MPD_OnNotifyException(pUser->addressID, flowId, exceptionCode);
        }
    }
}

#ifdef TRAFFIC_CTL
void MPD_DoSendWaitQueueData(stUserInfo* pUser)
{
	U8 *dataAddr;
	S32 length;
	U32 pktCntSend = 0;
    S32  result = SEND_FAIL;
    U8   flowId = INVALID_STREAM;
    bool isFragment = false;
    stDataFragmentRec dataFrag;
    stDataNode* pDataNode = NULL;
    U32 uid = pUser->uid;
	U32 pktCnt = 9;  //(pUser->throuputTotal + 99)/100;
	U32	queueIdx = pUser->pDataWaiteTail;

	if(pUser->isMpStopSched)  //?????????,本周期多流停止调度
	{
		MPDLOG(uid, LLV_DEBUG, "MPD_DoSendWaitQueueData(),wait queue head %d, tail %d, isMpStopSched", 
			pUser->pDataWaiteHead, pUser->pDataWaiteTail);
		return;
	}

	if(pUser->pDataWaiteHead == pUser->pDataWaiteTail) //队列空
	{
		//no data wait to send
		MPDLOG(uid, LLV_DEBUG, "MPD_DoSendWaitQueueData(),wait queue head %d, tail %d, no data in wait queue", 
			pUser->pDataWaiteHead, pUser->pDataWaiteTail);
		return;
	}

	//????????????
	if(pUser->isAllFlowTrafficCtl)  //所有的流都在流控状态
	{
		pktCnt = (pUser->throuputTotal + 99)/100;  //换算每10MS总共可发送多少个包
	}
		
	MPDLOG(uid, LLV_WARNING, "MPD_DoSendWaitQueueData(),wait queue head %d, tail %d, isAllFlowTrafficCtl %d, pktCnt %d, g_isTrafficCtl %d", 
		pUser->pDataWaiteHead, pUser->pDataWaiteTail, pUser->isAllFlowTrafficCtl, pktCnt,g_isTrafficCtl);
	
	while((pUser->pDataWaiteHead != pUser->pDataWaiteTail))	
	{
		dataAddr = pUser->dataWaitQueue[queueIdx].PData;
		length = pUser->dataWaitQueue[queueIdx].len;

		if(dataAddr == NULL)
		{
			//error, should not goto here
			MPDLOG(uid, LLV_ERROR, "MPD_DoSendWaitQueueData dataAddr get from the wait queue is null.");
			pUser->pDataWaiteTail = (pUser->pDataWaiteTail + 1)%DataWaitQueueSize;


			return;
		}
		
		//schd subflow 
		flowId = MPD_SCHD_ChooseStream(pUser);
		if (flowId == INVALID_STREAM)
		{
			MPDLOG(uid, LLV_ERROR, "MPD_SCHD_ChooseStream fail.");
			//MPD_MsgMemfree(dataAddr);
			//return SEND_FAIL;
			break;
		}
		pUser->lastPackageTime = g_time + MPD_IDLE_TH;
		
		//check data length
		if(length >= LEN_DATA_TH)
		{
			S32 rst = MPD_SplitDataMem(pUser, dataAddr, length, &dataFrag);
			if(rst == 0)
			{
				isFragment = true;
			}
		}
		
		MPDLOG(uid, LLV_DEBUG, "MPD_DoSendWaitQueueData(), schd flow %d isFragment %d", flowId, isFragment);
		//fill mpd header
		//put data to tx window
		//send data    
		if (!isFragment)
		{
			MPD_fillMPDHeader(pUser, flowId, MPD_GetMsgDataPtr(dataAddr, false), NO_FRAGMENT);
			pDataNode = MPD_putDataToTxWin(pUser, flowId, dataAddr,length);
			result = MPD_callMPCToSend(pUser, flowId, pDataNode);
			pktCntSend++;
		}
		else
		{
			//first frag
			MPD_fillMPDHeader(pUser, flowId, dataFrag.pFirstBlockDataAddr, FIRST_FRAG_BLK);
			pDataNode = MPD_putDataToTxWin(pUser, flowId, dataFrag.pFirstBlockAddr, dataFrag.firstBlockLength);
			result = MPD_callMPCToSend(pUser, flowId, pDataNode);
			//last frag
			MPDLOG(pUser->uid, LLV_WARNING,"MPD_DoSendWaitQueueData and set txDataSn--   %d", pUser->txDataSn);
			pUser->txDataSn--; //frags use the same DSN
			MPD_fillMPDHeader(pUser, flowId, dataFrag.pLastBlockDataAddr, LAST_FRAG_BLK);
			pDataNode = MPD_putDataToTxWin(pUser, flowId, dataFrag.pLastBlockAddr, dataFrag.lastBlockLength);
			result = MPD_callMPCToSend(pUser, flowId, pDataNode);
			pktCntSend = pktCntSend + 2;
		}

		pUser->dataWaitQueue[queueIdx].PData = NULL;
		queueIdx = (queueIdx + 1)%DataWaitQueueSize;
		pUser->pDataWaiteTail = queueIdx;

		if(pUser->isMpStopSched)
		{
			break;
		}

		if(pktCntSend >= pktCnt)
		{
			break;
		}

		if(result == SEND_FAIL)
		{
			break;
		}
	}

	return;

}


void MPD_clearWaitQueue(stUserInfo* pUser)
{	
	MPDLOG(pUser->uid, LLV_ERROR, "MPD_clearWaitQueue  ........");
	
	while(pUser->pDataWaiteHead != pUser->pDataWaiteTail)
	{
		//delete the tail node
		MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", pUser->dataWaitQueue[pUser->pDataWaiteTail].PData);
		MPD_MsgMemfree(pUser->dataWaitQueue[pUser->pDataWaiteTail].PData);	
		pUser->dataWaitQueue[pUser->pDataWaiteTail].PData = NULL;
		pUser->pDataWaiteTail = (pUser->pDataWaiteTail + 1)%DataWaitQueueSize;		
	}

	memset(pUser->dataWaitQueue, 0, sizeof(stWaitSendNode)*DataWaitQueueSize);
	pUser->pDataWaiteHead = 0;
	pUser->pDataWaiteTail = 0;
}
#endif

/*发送端最后一个包丢失，发端一直收不到该包的确认，直到释放该流。为了避免
该问题，在发端周期检测最后一个包在一定时间内没收到ACK确认，则主动重传*/
void MPD_TxWinRetransCheck(stUserInfo* pUser)
{
    int flowId;
    stTxWin* pTxWin;
    stDataNode* pTxNew;
    U16 toCnt;
    U32 tm;
    U32 rtt;

    if((g_time % 30) == (pUser->uid % 30))
    {
        for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
        {            
            if (!MPD_SCHD_getFlowValid(pUser, flowId))
                continue;

            pTxWin = &pUser->txWindow[flowId];
            if(pTxWin->timeOutRetCnt == 0)
            {
                pTxWin->timeOutRetCnt++;
                continue;
            }
            
            toCnt = pTxWin->timeOutRetCnt;
            if(pTxWin->pTxLast && (pTxWin->pTxLast->transState != DATA_INIT) && (toCnt < 4))  //重传次数从1开始计
            {
                rtt = pUser->schdPara[flowId].InhertDelay;  //ms
                tm = toCnt*rtt;  // /10*10
                tm = tm>>2;
                pTxNew = pTxWin->pTxLast;
                tm += pTxNew->T1;
                if(g_time > tm)
                {
                    //重传最新包

                    MPDLOG(pUser->uid, LLV_WARNING, "MPD_TxWinRetransCheck() timeout retrans flowId %d, rtt %d, T1 %d, tm %d;", 
                     flowId, rtt, pTxNew->T1,tm);
#ifndef MAG
                    MYLOGP(0xffff, LLV_WARNING,"MPD_TxWinRetransCheck() timeout retrans flowId %d, rtt %d, T1 %d, tm %d;", 
                     flowId, rtt, pTxNew->T1,tm);
#endif
                    MPD_callMPCToRetrans(pUser, flowId, pTxNew);
                    pTxNew->transState = SACKED;
                    
                    if(flowId == 0) //McWill
                        pTxNew->TO_retrans = g_time + RETRANS_TO_MC;  //10待定
                    else    
                        pTxNew->TO_retrans = g_time + RETRANS_TO_OTHER;  //
                    pTxWin->timeOutRetCnt++;
                }
            }
        }
    }
}

void MPD_clearTxWin(stUserInfo* pUser, U8 flowId)
{
	stTxWin* pTxWin = &(pUser->txWindow[flowId]);
    MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_clearTxWin() flowId %d", flowId);
	while (pTxWin->pTxBegin != pTxWin->pTxNext) //==, all dataNodes in window are confirmed
	{
		MPD_freeTxBeginData(pUser, flowId);
		MPD_txBeginInc(pUser, flowId);
	}
    memset(&pUser->txWindow[flowId], 0, sizeof(stTxWin));
    pUser->txWindow[flowId].pTxBegin = &(pUser->txWindow[flowId].dataNode[0]);
    pUser->txWindow[flowId].pTxNext  = &(pUser->txWindow[flowId].dataNode[0]);
}

#ifdef DATAWIN_TXBEGIN
U16 MPD_GetDataWinTxBegin(stUserInfo* pUser)
{
    U8 i;
    U16 txBegin = 0xffffffff;
    for(i=0;i<NUM_SUBFLOW;i++)
    {
        if(MPD_SCHD_getFlowValid(pUser, i))
        {
            txBegin = min(txBegin, pUser->txWindow[i].pTxBegin->DSN);
        }
    }
    return txBegin;
}
#endif

//just for test
void MPD_CheckTxWinPdata(stUserInfo* pUser)
{
    stDataNode* pTxBgn;
    U8  flowId = 0;
    stTxWin* pTxWin;

    for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
    {
        if(!pUser->schdPara[flowId].isValid) 
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
            continue;
        }
        pTxWin = &pUser->txWindow[flowId];
        pTxBgn = pTxWin->pTxBegin;
        while (pTxBgn != pTxWin->pTxNext)
        {
            if(pTxBgn->pData != pTxBgn->pData1)
            {
                MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error pData 0x%x, pData1 0x%x,SSN %d, sfId %d !", pTxBgn->pData, pTxBgn->pData1, pTxBgn->SSN, flowId);
            }
            
            if(pTxBgn ==  &pTxWin->dataNode[MAX_TX_WINDOW_SIZE-1])
            {
                pTxBgn = &pTxWin->dataNode[0];
            }
            else
            {
                pTxBgn++;
            }
        }
    }
}

#if (defined  SCHD_NEW || defined SCHD_BALANCE_NEW)
bool MPD_isTxWinEmpty(stUserInfo* pUser, U8 flowId)
{
    bool ret = false;
    if(pUser->txWindow[flowId].pTxBegin == pUser->txWindow[flowId].pTxNext && pUser->schdPara[flowId].isSendPkt)
    {
    	pUser->schdPara[flowId].isSendPkt = false;  //cwy add
        ret =  true;
    }
    //pUser->schdPara[flowId].isSendPkt = false;
    return ret;
}

#endif

#ifdef SCHD_DELAY
//待发送数据包个数
U32 MPD_getHistAcnt(stUserInfo* pUser, U8 flowId)
{
	stDataNode* pPrevNode;
	U32 cnt = 0;
	stTxWin* pTxWin  = &pUser->txWindow[flowId];
	U32 histOld = pTxWin->histA;
	U32 histNew = pTxWin->histA;

	if((pTxWin->pTxNext == pTxWin->pTxBegin) || (pTxWin->histA == 0))
	{		
		pTxWin->lastTxTime = g_time;
		pTxWin->histA = 0;
		return 0;
	}

	U32 txCnt = (g_time - pTxWin->lastTxTime)*pUser->schdPara[flowId].PktAvg/100;
	if(txCnt > 0)
	{
		if(pTxWin->histA > txCnt)
		{
			histNew = pTxWin->histA - txCnt;
		}
		else
		{
			histNew = 0;
		}
	}

	MPDLOG(pUser->uid, LLV_DEBUG, "MPD_getHistAcnt(), g_time %d,lastTxTime %d,pktAvg %d,histOld %d,histNew %d", 
		g_time, pTxWin->lastTxTime, pUser->schdPara[flowId].PktAvg, histOld, histNew);
	
	if(pTxWin->pTxNext == &pUser->txWindow[flowId].dataNode[0])
	{
		//printf("==\n");
		pPrevNode = &pUser->txWindow[flowId].dataNode[MAX_TX_WINDOW_SIZE-1];
	}
	else
	{
		pPrevNode = pTxWin->pTxNext -1;
	}

	cnt = pPrevNode->SSN - pTxWin->pTxBegin->SSN +1;  //计算待确认包个数计算量比较大

	if(histNew > cnt)
	{
		MPDLOG(pUser->uid, LLV_DEBUG, "MPD_getHistAcnt(), txWinDis %d,histOld %d,histNew %d", 
			cnt, histOld, histNew);
		histNew = cnt;
	}

	if(histNew != histOld)
	{
		pTxWin->lastTxTime = g_time; //
		pTxWin->histA = histNew;
	}

	return histNew;	
}
#endif

stDataNode* MPD_getTxWinPrevNode(stTxWin* pTxWin, stDataNode* pNode)
{
    if(pNode == &pTxWin->dataNode[0])
    {
        return &pTxWin->dataNode[MAX_TX_WINDOW_SIZE];
    }
    return pNode-1;
}

U32 MPD_getTxWinUnackCnt(stUserInfo* pUser, U8 flowId)
{
    stDataNode* pPrevNode;
    U32 cnt = 0;
    if(pUser->txWindow[flowId].pTxNext != pUser->txWindow[flowId].pTxBegin)
    {
        if(pUser->txWindow[flowId].pTxNext == &pUser->txWindow[flowId].dataNode[0])
        {
            //printf("==\n");
            pPrevNode = &pUser->txWindow[flowId].dataNode[MAX_TX_WINDOW_SIZE-1];
        }
        else
        {
            pPrevNode = pUser->txWindow[flowId].pTxNext -1;
        }
        /*
        MPDLOG(pUser->uid, LLV_INFO, "pTxNext is %x, next ssn is %d, pTxBegin is %x, begin ssn is %d, prev is %x, prev ssn is %d\n",
        pUser->txWindow[flowId].pTxNext,
        pUser->txWindow[flowId].pTxNext->SSN,
        pUser->txWindow[flowId].pTxBegin,
        pUser->txWindow[flowId].pTxBegin->SSN,
        pPrevNode,
        pPrevNode->SSN
        );
        */
        cnt = pPrevNode->SSN - pUser->txWindow[flowId].pTxBegin->SSN +1;
    }
    MPDLOG(pUser->uid, LLV_INFO, "TxWinUnackCnt flowId %d, cnt %d", flowId, cnt);
    return cnt;
}

#ifdef SCHD_BALANCE
U32 MPD_getTxWinRetransCnt(stUserInfo* pUser, U8 flowId)
{
    MPDLOG(pUser->uid, LLV_INFO, "TxWinRetransCnt flowId %d, cnt %d", flowId, pUser->txWindow[flowId].retransCnt);
    return pUser->txWindow[flowId].retransCnt;
}
#endif
