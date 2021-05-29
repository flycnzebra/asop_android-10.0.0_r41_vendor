/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_rx.c
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

#include "mpd_rx.h"
#include <stdlib.h>
#include "mpd.h"
#include "mpd_external.h"
/******************************************************************************/
#define SubWinPlus(n)  (n+1)%RxSubWinMaxSize
#define DataWinPlus(n)  (n+1)%RxDataWinMaxSize
#define SsnPlus(n)      (n+1)%MAX_SN
/******************************************************************************/
#ifndef MAG
extern RAT_Env g_mpcFun;
#endif

#ifdef RETRANS_OPTIMIZE
extern void MPD_processACK(stUserInfo* pUser, U8 sfId, U16 ackSSN, U8 firstSack);
#else
extern void MPD_processACK(stUserInfo* pUser, U8 sfId, U16 ackSSN);
#endif

#ifdef RETRANS_OPTIMIZE
extern void MPD_parserSACK(stUserInfo* pUser, U8 sfId, U8* pSack, U16 ack);
#else
extern void MPD_parserSACK(stUserInfo* pUser, U8 sfId, U8* pSack);
#endif

extern U8 MPD_SCHD_getFlowValid(stUserInfo* pUser, U8 flowId);
/******************************************************************************/

    
 
void MPD_RxWinInit(stUserInfo* pUser)
{
    //memset(pUser->RxSubWin,0,sizeof(stRxSubWin)*NUM_SUBFLOW);
    memset(pUser->RxSubWin,0,sizeof(stRxSubWin)*(NUM_SUBFLOW+1));
    memset(&pUser->RxDataWin,0,sizeof(stRxDataWin));
}


void MPD_SubWinRxBeginMove(stUserInfo* pUser, U8 SubWinIndex, bool forceMove)
{
    stRxSubWin* subWin = &pUser->RxSubWin[SubWinIndex];
    if(forceMove)
    {
        //至少移动一次
        subWin->RxBegin = SubWinPlus(subWin->RxBegin);
    }
    while(subWin->Win[subWin->RxBegin].status == Received && subWin->RxBegin != subWin->RxNext)
    {
        subWin->RxBegin = SubWinPlus(subWin->RxBegin);
    }
    MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] MPD_SubWinRxBeginMove SubWin%d RxBeginMoveTo %d,ssn %d", SubWinIndex, subWin->RxBegin, subWin->Win[subWin->RxBegin].SSN);
}

void MPD_SubWinRxNextMove(stUserInfo* pUser, U8 SubWinIndex)
{
    stRxSubWin* subWin = &pUser->RxSubWin[SubWinIndex];
    U16 ssn = subWin->Win[subWin->RxNext].SSN;
    subWin->RxNext = SubWinPlus(subWin->RxNext);
    if(subWin->RxNext == subWin->RxBegin)
    {
        MPD_SubWinRxBeginMove(pUser, SubWinIndex, true);
    }
    subWin->Win[subWin->RxNext].status = UnReceived;
    subWin->Win[subWin->RxNext].SSN = SsnPlus(ssn);    //ssn+1;
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_SubWinRxNextMove SubWin%d RxNextMoveTo %d,ssn %d", SubWinIndex, subWin->RxNext, subWin->Win[subWin->RxNext].SSN);
}

#ifdef RETRANS_OPTIMIZE
void MPD_UpdateSubwinBegin(stUserInfo* pUser)
{
    U8 i;
        
    for(i = 0;i < NUM_SUBFLOW; i++)
    {
        while((pUser->RxSubWin[i].RxBegin<=pUser->RxSubWin[i].dataWinSSN) && (pUser->RxSubWin[i].RxBegin!= pUser->RxSubWin[i].RxNext))
        {
            MPD_SubWinRxBeginMove(pUser, i, true);
        }
    }
}
#endif
void MPD_SubWinAddNode(stUserInfo* pUser, stMPDNodeInfo* node)
{
    stRxSubWin* subWin = &pUser->RxSubWin[node->stream];
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_SubWinAddNode SubWin%d AddNode,ssn %d, dsn %d", node->stream, node->SSN, node->DSN);
    U16 nodeSSN = node->SSN;
    U16 beginSSN = subWin->Win[subWin->RxBegin].SSN;
    U16 nextSSN = subWin->Win[subWin->RxNext].SSN;
/*
    if(subWin->RxBegin == subWin->RxNext)
    {
        //空窗，直接放入
        subWin->RxBegin = node->SSN%RxSubWinMaxSize;
        subWin->Win[subWin->RxBegin].SSN = node->SSN;
        subWin->Win[subWin->RxBegin].status = Received;
        SubWinRxNextMove(node->stream);
        return;
    }
*/
#ifdef SSN_OPTIMIZE
if((subWin->RxBegin == 0) && (subWin->RxNext== 0))
{
	if(subWin->Win[subWin->RxBegin].SSN == 0)  //接收窗已清0
	{
		subWin->RxBegin = subWin->RxNext = node->SSN%RxSubWinMaxSize;
		subWin->Win[subWin->RxBegin].status = UnReceived;
		subWin->Win[subWin->RxBegin].SSN = nodeSSN;
		beginSSN = nextSSN = nodeSSN;
	}
}

if(DISTANCESN(nodeSSN,beginSSN)<DISTANCESN(beginSSN, nodeSSN))//begin左边
{
    //已收包或者强制移窗
    MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Data out of SubWindow beginSSN %d, nextSSN %d, nodeSSN %d", beginSSN, nextSSN, nodeSSN);
} 
else if(DISTANCESN(nodeSSN,nextSSN)>=DISTANCESN(nextSSN, nodeSSN))//next右边
{
    while(node->SSN != subWin->Win[subWin->RxNext].SSN)
    {
        MPD_SubWinRxNextMove(pUser, node->stream);
        subWin->Win[subWin->RxNext].status = UnReceived;
    }
    subWin->Win[subWin->RxNext].status = Received;
    MPD_SubWinRxNextMove(pUser, node->stream);
}
else
{
    subWin->Win[node->SSN%RxSubWinMaxSize].status = Received;
}

#else
    if(node->SSN < subWin->Win[subWin->RxBegin].SSN && ((node->SSN + MaxSSN - subWin->Win[subWin->RxBegin].SSN)>RxSubWinMaxSize/2))
    {
        //已收包或者强制移窗
        MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Data out of SubWindow");
    }
    else if(node->SSN >= subWin->Win[subWin->RxBegin].SSN && node->SSN < subWin->Win[subWin->RxNext].SSN)
    {
        subWin->Win[node->SSN%RxSubWinMaxSize].status = Received;
    }
    else
    {
        while(node->SSN != subWin->Win[subWin->RxNext].SSN)
        {
            MPD_SubWinRxNextMove(pUser, node->stream);
            subWin->Win[subWin->RxNext].status = UnReceived;
        }
        subWin->Win[subWin->RxNext].status = Received;
        MPD_SubWinRxNextMove(pUser, node->stream);
    }
#endif
    MPD_SubWinRxBeginMove(pUser, node->stream, false);
    return;
}

void MPD_DataWinFreeMsgPtr(stUserInfo* pUser, U16 index)
{
    U8 i;
    U8 frag = pUser->RxDataWin.Win[index].fragment;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    if(rxDataWin->Win[index].status == UnReceived)
    {
        MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinFreeMsgPtr,UnReceived,index %d,Dsn %d,frag %d", index, rxDataWin->Win[index].DSN, rxDataWin->Win[index].fragment);
        return;
    }
    
    if(frag == NoFrag)
    {
        if(rxDataWin->Win[index].msgPtr[0] != NULL)
        {
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", rxDataWin->Win[index].msgPtr[0]);
            MPD_MsgMemfree(rxDataWin->Win[index].msgPtr[0]);
            MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinFreeMsgPtr,index %d,Frag %d,Ptr %d", index, frag, 0);
        }
        else
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinFreeMsgPtrError,Ptr NULL,index %d,Frag %d,Ptr %d", index, frag, 0);
        }
    }
    else
    {
        for(i=0;i<MaxFragNum;i++)
        {
            if(frag&(1<<i))
            {
                if(rxDataWin->Win[index].msgPtr[i] != NULL)
                {
                    MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", rxDataWin->Win[index].msgPtr[i]);
                    MPD_MsgMemfree(rxDataWin->Win[index].msgPtr[i]);
                    MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinFreeMsgPtr,index %d,Frag %d,Ptr %d", index, frag, i);
                }
                else
                {
                    MPDLOG(pUser->uid, LLV_ERROR, "[MPDRX] DataWinFreeMsgPtrError,Ptr NULL,index %d,Frag %d,Ptr %d", index, frag, i);
                }
            }
        }
    }
}

void MPD_DataWinRxBeginMove(stUserInfo* pUser, bool forceMove)
{
    U8 i = 0;
    U16 len ;
    U8* msgPtr;
    U8* dataPtr;
    U8 mpDataOffset = 0;
    U16 tempRxbegin;
    U16 subRxBeginSsn;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    tempRxbegin = rxDataWin->RxBegin;
	if(forceMove)
	{
		//至少移动一次
	    if(rxDataWin->Win[rxDataWin->RxBegin].status == UnReceived)
	    {
	        rxDataWin->RxBegin = DataWinPlus(rxDataWin->RxBegin);
	    }
	    else if(rxDataWin->Win[rxDataWin->RxBegin].status == Received)
	    {
            MPD_DataWinFreeMsgPtr(pUser, rxDataWin->RxBegin);
	        rxDataWin->RxBegin = DataWinPlus(rxDataWin->RxBegin);
	    }
	}
    //检查RxBegin是否可以继续移动
    while(rxDataWin->Win[rxDataWin->RxBegin].status == ReceiveComplete || rxDataWin->Win[rxDataWin->RxBegin].timeOut)
    {
        if(rxDataWin->Win[rxDataWin->RxBegin].status == ReceiveComplete)
        {
            mpDataOffset = IPUDPHeadOffset + rxDataWin->Win[rxDataWin->RxBegin].mpdHeadLen;
            if(rxDataWin->Win[rxDataWin->RxBegin].fragment == NoFrag)
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] MPD_DataWinRxBeginMove DataWinPostMPDataToTransport,index %d,dsn %d,Frag %d,addr %x", rxDataWin->RxBegin, rxDataWin->Win[rxDataWin->RxBegin].DSN, rxDataWin->Win[rxDataWin->RxBegin].fragment,rxDataWin->Win[rxDataWin->RxBegin].msgPtr[0]);
#ifdef CHECK_ACK
                if(rxDataWin->Win[rxDataWin->RxBegin].flag!=Sended)
                    MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)rxDataWin->Win[rxDataWin->RxBegin].msgPtr[0], rxDataWin->Win[rxDataWin->RxBegin].msgLen[0], mpDataOffset);
#else                
                MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)rxDataWin->Win[rxDataWin->RxBegin].msgPtr[0], rxDataWin->Win[rxDataWin->RxBegin].msgLen[0], mpDataOffset);
#endif

#ifdef RETRANS_OPTIMIZE
				if(MPD_SCHD_getFlowValid(pUser, rxDataWin->Win[rxDataWin->RxBegin].stream))
                pUser->RxSubWin[rxDataWin->Win[rxDataWin->RxBegin].stream].dataWinSSN = SsnPlus(rxDataWin->Win[rxDataWin->RxBegin].SSN);   //rxDataWin->Win[rxDataWin->RxBegin].SSN;
#endif

            }
            else if(rxDataWin->Win[rxDataWin->RxBegin].fragment == AllFrag)
            {
                //拼包发送
                MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinPostMPDataToTransport,index %d,dsn %d,Frag %d", rxDataWin->RxBegin, rxDataWin->Win[rxDataWin->RxBegin].DSN, rxDataWin->Win[rxDataWin->RxBegin].fragment);
                len = mpDataOffset;
                for(i=0;i<MaxFragNum;i++)
                {
                    len += rxDataWin->Win[rxDataWin->RxBegin].msgLen[i] - mpDataOffset;
                }
                msgPtr = MPD_MsgMemCreat(len);
                MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemCreat1 address 0x%x ", msgPtr);
                if(msgPtr != NULL)
                {
                    dataPtr = MPD_GetMsgDataPtr(msgPtr, false);
                    memcpy(dataPtr, MPD_GetMsgDataPtr(rxDataWin->Win[rxDataWin->RxBegin].msgPtr[0], false), mpDataOffset);
                    len = mpDataOffset;
                    for(i=0;i<MaxFragNum;i++)
                    {
                        memcpy(dataPtr+len, MPD_GetMsgDataPtr(rxDataWin->Win[rxDataWin->RxBegin].msgPtr[i], false)+mpDataOffset, rxDataWin->Win[rxDataWin->RxBegin].msgLen[i] - mpDataOffset);
                        len += rxDataWin->Win[rxDataWin->RxBegin].msgLen[i] - mpDataOffset;
                    }
                    MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)msgPtr, len, mpDataOffset);
                }
#ifdef RETRANS_OPTIMIZE
				if(MPD_SCHD_getFlowValid(pUser, rxDataWin->Win[rxDataWin->RxBegin].stream))
                pUser->RxSubWin[rxDataWin->Win[rxDataWin->RxBegin].stream].dataWinSSN = SsnPlus(rxDataWin->Win[rxDataWin->RxBegin].SSN); //rxDataWin->Win[rxDataWin->RxBegin].SSN;
#endif
                //内存释放
                MPD_DataWinFreeMsgPtr(pUser, rxDataWin->RxBegin);
            }
        }
        else //超时
        {
            MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] warning, MPD_DataWinRxBeginMove RxBegin %d timeout!", rxDataWin->RxBegin);
            MPD_DataWinFreeMsgPtr(pUser, rxDataWin->RxBegin);
        }
        rxDataWin->RxBegin = DataWinPlus(rxDataWin->RxBegin);
    }
    if(tempRxbegin != rxDataWin->RxBegin)
    {
        MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] DataWin RxBeginMove From %d To %d,Dsn %d", tempRxbegin, rxDataWin->RxBegin, rxDataWin->Win[rxDataWin->RxBegin].DSN);
    }

    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_DataWinRxBeginMove From %d To %d,Nxt %d", tempRxbegin, rxDataWin->RxBegin, rxDataWin->RxNext);
#ifdef RETRANS_OPTIMIZE
    for(i=0;i<NUM_SUBFLOW;i++)
    {
		if(MPD_SCHD_getFlowValid(pUser, i) == 0)
			continue;

        subRxBeginSsn = pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN;
        if(DISTANCESN(subRxBeginSsn, pUser->RxSubWin[i].dataWinSSN) > 
            DISTANCESN(subRxBeginSsn, pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxNext].SSN))
        {
            MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] ERROR... MPD_DataWinRxBeginMove flowId %d beging %d next %d,dataWinSSN %d", 
                i,subRxBeginSsn, pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxNext].SSN, pUser->RxSubWin[i].dataWinSSN);
            continue;
        }
        //if((pUser->RxSubWin[i].dataWinSSN == 0)&& (pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN == 0)) //认为是初始值，不处理
            //continue;
        
        //while(pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN != pUser->RxSubWin[i].dataWinSSN)
        //while((DISTANCESN(pUser->RxSubWin[i].dataWinSSN,pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN) >= DISTANCESN(pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN, pUser->RxSubWin[i].dataWinSSN)) && 
            //(DISTANCESN(pUser->RxSubWin[i].dataWinSSN,pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxNext].SSN) < DISTANCESN(pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxNext].SSN, pUser->RxSubWin[i].dataWinSSN)))
        //while((DISTANCESN(pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN, pUser->RxSubWin[i].dataWinSSN) > 0) && 
        while((pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN != pUser->RxSubWin[i].dataWinSSN) &&     
            (DISTANCESN(pUser->RxSubWin[i].Win[pUser->RxSubWin[i].RxBegin].SSN, pUser->RxSubWin[i].dataWinSSN) < RCV_SN_RANGE))
        {   
            pUser->RxSubWin[i].RxBegin = SubWinPlus(pUser->RxSubWin[i].RxBegin);
        }

        MPD_SubWinRxBeginMove(pUser, i, false);
        
    }
#endif
}

void MPD_DataWinRxNextMove(stUserInfo* pUser)
{
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    U16 dsn = rxDataWin->Win[rxDataWin->RxNext].DSN;
    rxDataWin->RxNext = DataWinPlus(rxDataWin->RxNext);
    if(rxDataWin->RxNext == rxDataWin->RxBegin)
    {
        MPD_DataWinRxBeginMove(pUser, true);
    }
    memset(&rxDataWin->Win[rxDataWin->RxNext], 0, sizeof(stRxDataWinNode));
    rxDataWin->Win[rxDataWin->RxNext].DSN = dsn+1;
    MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWin RxNextMoveTo rxnext %d,rxnextDsn %d, rxBegin %d", rxDataWin->RxNext, rxDataWin->Win[rxDataWin->RxNext].DSN, rxDataWin->RxBegin);
}

void MPD_DataWinPutInNode(stUserInfo* pUser, U16 index,stMPDNodeInfo* node)
{
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
#ifdef CHECK_ACK
    rxDataWin->Win[index].flag = UnChecked;
    //rxDataWin->NewData= false;
#endif    
    rxDataWin->Win[index].status = Received;
    if(node->fragment == NoFrag)
    {
        rxDataWin->Win[index].fragment = NoFrag;
        rxDataWin->Win[index].status = ReceiveComplete;
#ifdef CHECK_ACK
        rxDataWin->NewData= true;
#endif        
    }
    else
    {
        rxDataWin->Win[index].fragment |= 1<<(node->fragment-1);//从bit0开始，每一位代表一个分片
        if(rxDataWin->Win[index].fragment == AllFrag)
        {
            rxDataWin->Win[index].status = ReceiveComplete;
        }
        else
        {
            rxDataWin->Win[index].Time = g_time + DataWinWaitTimeOut;
        }
    }
    rxDataWin->Win[index].DSN = node->DSN;
    rxDataWin->Win[index].mpdHeadLen = node->mpdHeadLen;
#ifdef RETRANS_OPTIMIZE
    rxDataWin->Win[index].stream = node->stream;
    rxDataWin->Win[index].SSN = node->SSN;
#endif
    if(node->fragment == NoFrag)
    {
        rxDataWin->Win[index].msgPtr[0] = node->msgPtr;
        rxDataWin->Win[index].msgLen[0] = node->msgLen;
    }
    else
    {
        rxDataWin->Win[index].msgPtr[node->fragment-FirstFrag] = node->msgPtr;
        rxDataWin->Win[index].msgLen[node->fragment-FirstFrag] = node->msgLen;
    }
    MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWin PutInNode index %d,Dsn %d,Frag %d", index, rxDataWin->Win[index].DSN, rxDataWin->Win[index].fragment);
}

//void MPD_DataWinAddNode(stUserInfo* pUser, stMPDNodeInfo* node)
U32 MPD_DataWinAddNode(stUserInfo* pUser, stMPDNodeInfo* node)
{
    U32 index = node->DSN%RxDataWinMaxSize;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    U16 nodeDSN = node->DSN;
    U16 beginDSN = rxDataWin->Win[rxDataWin->RxBegin].DSN;
    U16 nextDSN = rxDataWin->Win[rxDataWin->RxNext].DSN;
    stRxSubWin* rxSubWin = NULL;
    U32 ret = 1;
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_DataWinAddNode,dsn %d,fragment %d, begin %d, next %d", node->DSN, node->fragment,beginDSN, nextDSN);
/*
    if(RxDataWin.RxBegin == RxDataWin.RxNext)
    {
        //空窗，直接放入
        memset(&RxDataWin.Win[index], 0, sizeof(stRxDataWinNode));
        RxDataWin.RxNext = RxDataWin.RxBegin = index;
        MPD_DataWinPutInNode(index,node);
        MPD_DataWinRxNextMove();
        MPD_DataWinRxBeginMove(false);
        return;
    }
*/
    if((rxDataWin->RxBegin == 0) && (rxDataWin->RxNext == 0))
    {
        //空窗，直接放入
        if(rxDataWin->Win[rxDataWin->RxBegin].DSN == 0)
    	{
		    rxDataWin->RxNext = rxDataWin->RxBegin = index;
			rxDataWin->Win[rxDataWin->RxBegin].DSN = nodeDSN;
			rxDataWin->Win[rxDataWin->RxBegin].flag = UnReceived;
			beginDSN = nextDSN = nodeDSN;
    	}
    }

#ifdef SSN_OPTIMIZE
    if(DISTANCESN(nodeDSN,beginDSN)<DISTANCESN(beginDSN, nodeDSN))//begin左边
    {
        if(DISTANCESN(nodeDSN,beginDSN)<=1000)//之前是500
        {
            MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] Out Of WIN DataWinPostMPDataToTransport,ssn %d,dsn %d,Frag %d,addr %x", node->SSN, node->DSN, node->fragment, node->msgPtr);
            MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)node->msgPtr,node->msgLen, node->mpdHeadLen);
            rxSubWin = &(pUser->RxSubWin[node->stream]);
            rxSubWin->dataWinSSN = SsnPlus(node->SSN);
            //pUser->rxDsnErr++;
            //ret = 1;
        }
        else
        {
            //已收包或者强制移窗，直接释放
            MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Data out of DataWindow, DSN %d, Rxbegin Dsn %d", node->DSN, rxDataWin->Win[rxDataWin->RxBegin].DSN);
            MYLOG(0xffff, LLV_WARNING,"[MPDRX] MPD RCV Error1, buffer 0x%x", node->msgPtr);
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", node->msgPtr);
            pUser->rxDsnErr++;
            MPD_MsgMemfree(node->msgPtr);
            ret = 0;
        }
        //pUser->rxDsnErr++;
    } 
    else if(DISTANCESN(nodeDSN,nextDSN)>=DISTANCESN(nextDSN, nodeDSN))//next右边
    {
        if(DISTANCESN(nextDSN, nodeDSN) > 1999)
        {
            MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Data out of DataWindow, DSN %d, RxNext Dsn %d", node->DSN, rxDataWin->Win[rxDataWin->RxNext].DSN);
            MYLOG(0xffff, LLV_WARNING,"[MPDRX] MPD RCV Error2, buffer 0x%x", node->msgPtr);
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", node->msgPtr);
            MPD_MsgMemfree(node->msgPtr);
            ret = 0;
            pUser->rxDsnErr++;
        }
        else
        {
            while(node->DSN != rxDataWin->Win[rxDataWin->RxNext].DSN)
            {
                rxDataWin->Win[rxDataWin->RxNext].Time = g_time + DataWinWaitTimeOut;
                MPD_DataWinRxNextMove(pUser);
            }
            MPD_DataWinPutInNode(pUser, rxDataWin->RxNext,node);
            MPD_DataWinRxNextMove(pUser);
            pUser->rxDsnErr = 0;
        }
    }
    else
    {
        if((rxDataWin->Win[index].status == Received && (node->fragment & rxDataWin->Win[index].fragment))
            || rxDataWin->Win[index].status == ReceiveComplete)
        {
        
                MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Duplicate Packet");
                MYLOG(0xffff, LLV_WARNING,"[MPDRX] MPD RCV Error3, buffer 0x%x", node->msgPtr);
                MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", node->msgPtr);
                MPD_MsgMemfree(node->msgPtr);
                ret = 0;
        }
        else
        {
            MPD_DataWinPutInNode(pUser, index, node);
        }
        pUser->rxDsnErr = 0;
    }

#else
    if(node->DSN < rxDataWin->Win[rxDataWin->RxBegin].DSN && ((node->DSN + MaxDSN - rxDataWin->Win[rxDataWin->RxBegin].DSN) > RxDataWinMaxSize/2))
    {
#ifdef test    
        if(DISTANCESN(nodeDSN, beginDSN) <= 100)
        {
            MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] Out Of WIN DataWinPostMPDataToTransport,ssn %d,dsn %d,Frag %d,addr %x", node->SSN, node->DSN, node->fragment, node->msgPtr);
            MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)node->msgPtr,node->msgLen, node->mpdHeadLen);
        }
        else
#endif            
        {
            //已收包或者强制移窗，直接释放
            MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Data out of DataWindow, DSN %d, Rxbegin Dsn %d", node->DSN, rxDataWin->Win[rxDataWin->RxBegin].DSN);
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", node->msgPtr);
            MPD_MsgMemfree(node->msgPtr);
        }
    }
    else if(node->DSN >= rxDataWin->Win[rxDataWin->RxBegin].DSN && node->DSN < rxDataWin->Win[rxDataWin->RxNext].DSN)
    {
        if((rxDataWin->Win[index].status == Received && (node->fragment & rxDataWin->Win[index].fragment))
            || rxDataWin->Win[index].status == ReceiveComplete)
        {
 
                MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] Duplicate Packet");
                MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", node->msgPtr);
                MPD_MsgMemfree(node->msgPtr);
        }
        else
        {
            MPD_DataWinPutInNode(pUser, index, node);
        }
    }
    else
    {
        while(node->DSN != rxDataWin->Win[rxDataWin->RxNext].DSN)
        {
            rxDataWin->Win[rxDataWin->RxNext].Time = g_time + DataWinWaitTimeOut;
            MPD_DataWinRxNextMove(pUser);
        }
        MPD_DataWinPutInNode(pUser, rxDataWin->RxNext,node);
        MPD_DataWinRxNextMove(pUser);
    }
#endif    
    //MPD_DataWinRxBeginMove(pUser, false);
    if(pUser->rxDsnErr > 100)
    {
        MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_DataWinAddNode pUser->rxDsnErr %d", pUser->rxDsnErr);
        MPD_OnNotifyException(pUser->addressID, 0, -4);
        pUser->rxDsnErr = 0;
    }
    
    return ret;
}


S32 MPD_DoParseMPData (stUserInfo* pUser, const char* buffer,U32 datalen)
{
    stMPDNodeInfo mpdNode;
    U32 dataWinRet = 0;
    U8* dataPtr = MPD_GetMsgDataPtr(buffer, false);
    U8 mpdHeadLen = 0;
    stMPDHeader* mpdHead = (stMPDHeader*)(dataPtr+IPUDPHeadOffset);
    //MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] MPD_DoParseMPData");
#ifdef MAG
    MPD_CheckTxWinPdata(pUser);
#endif
    
    if(mpdHead->flag != MPD_PACKTET_FLAG|| (!MPD_SCHD_getFlowValid(pUser, mpdHead->stream)))
    {
	    MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] MPD Packet Flag Error, flowID %d, flag %d", mpdHead->stream, mpdHead->flag);
        MYLOGP(0xffff, LLV_WARNING,"[MPDRX] MPD RCV Error, flowID %d, flag %d, buffer 0x%x", mpdHead->stream, mpdHead->flag, buffer);
        //if(mpdHead->flag == MPD_PACKTET_FLAG)
        {
            MYLOG(0xffff, LLV_WARNING,"ahead:%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x ", 
                buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8],buffer[9],buffer[10]);
            MYLOG(0xffff, LLV_WARNING,"tail:%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x ", 
                buffer[datalen-11],buffer[datalen-10],buffer[datalen-9],buffer[datalen-8],buffer[datalen-7],buffer[datalen-6],buffer[datalen-5],buffer[datalen-4],buffer[datalen-3],buffer[datalen-2],buffer[datalen-1]);
            MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", buffer);
            MPD_MsgMemfree(buffer);
        }
        return -1;
    }
    pUser->lastPackageTime = g_time + MPD_IDLE_TH;
    mpdNode.DSN = (mpdHead->DSN[0]<<8)+mpdHead->DSN[1];
    mpdNode.SSN = (mpdHead->SSN[0]<<8)+mpdHead->SSN[1];
    mpdNode.fragment = mpdHead->fragment;
    mpdNode.stream = mpdHead->stream;
    mpdNode.msgPtr = (U8*)buffer;
    mpdNode.msgLen = datalen;
    mpdNode.mpdHeadLen = mpdHead->length*4;
    //pUser->RxSubWin[mpdHead->stream].Ack = mpdHead->ack;
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] SF%d RevMPD,SSN %d,DSN %d,Frag %d,Len %d,ack %d,mpdHeadLen %d,dataAddr %x", 
        mpdNode.stream, mpdNode.SSN, mpdNode.DSN, mpdNode.fragment, datalen, (mpdHead->ack[0]<<8)+mpdHead->ack[1],mpdNode.mpdHeadLen, buffer);
#ifndef MAG    
    MYLOGP(0xffff, LLV_INFO,"[MPDRX] SF%d RevMPD,SSN %d,DSN %d,Len %d,ack %d,mpdHeadLen %d,dataAddr %x", 
        mpdNode.stream, mpdNode.SSN, mpdNode.DSN, datalen, (mpdHead->ack[0]<<8)+mpdHead->ack[1],mpdNode.mpdHeadLen, buffer);
#endif

#ifndef RETRANS_OPTIMIZE
    MPD_processACK(pUser, mpdNode.stream, (mpdHead->ack[0]<<8)+mpdHead->ack[1]);
#endif
    if(mpdNode.mpdHeadLen == datalen-IPUDPHeadOffset)
    {
        if((mpdNode.mpdHeadLen - sizeof(stMPDHeader))>= 8)//四字节对齐，携带sack需要至少8个字节
        {
    	    MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] Receive Sack, mpdHeadLen %d", mpdNode.mpdHeadLen);
#ifdef RETRANS_OPTIMIZE               
            MPD_processACK(pUser, mpdNode.stream, (mpdHead->ack[0]<<8)+mpdHead->ack[1], *((U8*)mpdHead+sizeof(stMPDHeader)+2));
#endif
#ifdef RETRANS_OPTIMIZE               
            MPD_parserSACK(pUser, mpdNode.stream, (U8*)mpdHead+sizeof(stMPDHeader), (mpdHead->ack[0]<<8)+mpdHead->ack[1]);
#else
            MPD_parserSACK(pUser, mpdNode.stream, (U8*)mpdHead+sizeof(stMPDHeader));

#endif
        }
        else
        {
#ifdef RETRANS_OPTIMIZE               
            MPD_processACK(pUser, mpdNode.stream, (mpdHead->ack[0]<<8)+mpdHead->ack[1], 1);
#endif
            MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] Receive ack");
        }
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", buffer);
        MPD_MsgMemfree(buffer);
#ifdef MAG
            MPD_CheckTxWinPdata(pUser);
#endif
        
        return 0;
    }
#ifdef RETRANS_OPTIMIZE    
    else
    {
        //todo, adjust if the restrant pct    
        MPD_processACK(pUser, mpdNode.stream, (mpdHead->ack[0]<<8)+mpdHead->ack[1], 1);
    }
#endif
    //pUser->RxSubWin[mpdHead->stream].NeedSendSack = true;
    stRxSubWin* subWin = &pUser->RxSubWin[mpdNode.stream];
    if((subWin->RxBegin == 0) && (subWin->RxNext== 0))
    {
        if(subWin->Win[subWin->RxBegin].SSN == 0)  //接收窗已清0
        {
            //if(mpdNode.SSN)
            if(mpdNode.SSN > 5)    
            {
                MPDLOG(pUser->uid, LLV_ERROR, "MPD_DoParseMPData Error, sfID %d the rcvWin is clear but mpdNode.SSN %d", mpdNode.stream, mpdNode.SSN);
                MYLOG(0xffff, LLV_WARNING,"[MPDRX] MPD RCV Error, buffer 0x%x,sfID %d the rcvWin is clear but mpdNode.SSN %d", buffer, mpdNode.stream, mpdNode.SSN);
                MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", buffer);
                MPD_MsgMemfree(buffer);
#ifdef MAG
                    MPD_CheckTxWinPdata(pUser);
#endif
                
                return 0;
            }
        }
    }
    
    //MPD_SubWinAddNode(pUser, &mpdNode);  
#ifndef NO_RANK    
    dataWinRet = MPD_DataWinAddNode(pUser, &mpdNode); //返回0表示数据包由于某原因未放入数据接收窗
#else
    MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] DataWinPostMPDataToTransport,index %d,dsn %d,Frag %d,addr %x", mpdNode.SSN, mpdNode.DSN, mpdNode.fragment, mpdNode.msgPtr);
    MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)mpdNode.msgPtr,mpdNode.msgLen, mpdNode.mpdHeadLen);
#endif

    if(dataWinRet > 0)
    {
        MPD_SubWinAddNode(pUser, &mpdNode); 
        MPD_DataWinRxBeginMove(pUser, false);
    }
#ifdef MAG
        MPD_CheckTxWinPdata(pUser);
#endif
    
    return 0;
}

void MPD_DataWinTimeOutCheck(stUserInfo* pUser)
{
    U32 i=0;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    U16 nodeIndex = rxDataWin->RxBegin;
    U16 nodeCnt = (rxDataWin->RxNext+RxDataWinMaxSize-rxDataWin->RxBegin)%RxDataWinMaxSize;
    U8 status;
    U8 frag;
    U16 toCnt = 0;

    for(i=0; i<nodeCnt; i++)
    {
        status = rxDataWin->Win[nodeIndex].status;
        frag = rxDataWin->Win[nodeIndex].fragment;
        if(status != ReceiveComplete)
        {   
            if(g_time >= rxDataWin->Win[nodeIndex].Time)   //  == rxDataWin->Win[nodeIndex].Time)
            {
                toCnt++;
                rxDataWin->Win[nodeIndex].timeOut = true;
                MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] DataWinNodeTimeOut,index %d, Dsn %d, flag %d", nodeIndex, rxDataWin->Win[nodeIndex].DSN, rxDataWin->Win[nodeIndex].fragment);
            }
            else
            {
                break;
            }
        }
        nodeIndex = DataWinPlus(nodeIndex);
    }

    if(toCnt > 0)
        MPD_DataWinRxBeginMove(pUser, false);
    
}
#ifdef RETRANS_OPTIMIZE
void MPD_SubWinSendSackPacketMsg(stUserInfo* pUser, U16 ack, U8*sack ,U8 num, U8 index, bool firstSack)
#else
void MPD_SubWinSendSackPacketMsg(stUserInfo* pUser, U16 ack, U8*sack ,U8 num, U8 index)
#endif
{
    U8 msgLen; 
    U8 sackLen = 0;   
    U8* msg = NULL;
    U8* msgDataPtr;
    U8 i=0;
    stSack* pSack;
    stSackV* ptr;
    stMPDHeader* head;
    if(num != 0)
    {
        sackLen = sizeof(stSack) + (num-1)*sizeof(stSackV);
    }
    msgLen = sizeof(stMPDHeader) + sackLen;
    msgLen = (msgLen+3)/4*4;//4字节对齐
    msg = MPD_MsgMemCreat(msgLen);
    MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemCreat address 0x%x ", msg);
    if(msg != NULL)
    {
        msgDataPtr = MPD_GetMsgDataPtr(msg, false);
        head = (stMPDHeader*)msgDataPtr;
        pSack = (stSack*)((U8*)msgDataPtr + sizeof(stMPDHeader));
        head->flag = MPD_PACKTET_FLAG;
        head->length = msgLen/4;//长度代表4的倍数，向上取整
        head->stream = index;
        head->fragment = 0;
        head->ack[0] = ack>>8;
        head->ack[1] = ack;
        head->uid[0] = pUser->uid>>24;
        head->uid[1] = pUser->uid>>16;
        head->uid[2] = pUser->uid>>8;
        head->uid[3] = pUser->uid;
        head->DSN[0] = head->DSN[1] = 0;
        head->SSN[0] = head->SSN[1] = 0;
        if(num != 0)
        {
            pSack->Tag = SackTag;
            pSack->Len = sackLen;
#ifdef RETRANS_OPTIMIZE		
			pSack->Rsv[0] = firstSack;
#endif
            memcpy(&(pSack->Val.SackL[0]), sack, num*sizeof(stSackV));
            ptr = (stSackV*)&pSack->Val.SackL[0];
            for(i=0;i<num;i++)
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] Sack %d, SackL %d,SackR %d", i, (ptr[i].SackL[0]<<8)+ptr[i].SackL[1], (ptr[i].SackR[0]<<8)+ptr[i].SackR[1]);
            }
        }
        MPD_OnSendMPDataComplete(pUser->addressID, MPD_GetDeviceId(index), (const char*)msg, msgLen,1);
        pUser->txWindow[index].lastSendAckTime = g_time;
        MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] SubWinSendSackPacketMsg,index %d, ack %d, sackNum %d, mpdLen %d", index, ack, num, msgLen/4);
        #ifndef MAG
        //MYLOG(0xffff, LLV_WARNING,"[MPDRX] SubWinSendSackPacketMsg,index %d, ack %d, sackNum %d, mpdLen %d", index, ack, num, msgLen/4);
        #endif
    }
}

void MPD_SubWinCreateSack(stUserInfo* pUser, U8 index)
{
    U8 sackNum = 0;
    stSackV sack[MAX_SACK_NUM];
    U8 status = 0;
    U16 temp = 0;
    stRxSubWin* rxSubWin = &pUser->RxSubWin[index];
    U16 ack = rxSubWin->Win[rxSubWin->RxBegin].SSN;
    U16 rxbegin = rxSubWin->RxBegin;
    U16 rxnext = rxSubWin->RxNext;
    stTxWin*  pTxWin  = &pUser->txWindow[index];
    
#ifdef RETRANS_OPTIMIZE
	U8 firstSack = 1;
#endif
	//if((!rxSubWin.NeedSendSack) || (rxSubWin.RxBegin == rxSubWin.RxNext && ack == rxSubWin.SentAck))
    if(rxSubWin->RxBegin == rxSubWin->RxNext && ack == rxSubWin->SentAck)
    {
        //MPDLOG(pUser->uid, LLV_DEBUG, "[MPDRX] MPD_SubWinCreateSack,index %d, RxBegin %d, RxNext %d, SentAck %d", index, rxSubWin[index].RxBegin, rxSubWin[index].RxNext, rxSubWin[index].SentAck);
        if(g_time < (pTxWin->lastSendPktTime + 100))
        {
            pUser->totalToAckNum = 0;
            return;
        }
        
        if(g_time < (pTxWin->lastSendAckTime + 100))
        {
            return;
        }

        pUser->totalToAckNum++;
        if(pUser->totalToAckNum > (pUser->activeFlowNum*2))          //5)  // 15)
        {
            return;
        }
    }
    else
    {
        pUser->totalToAckNum = 0;
    }
/*
    //if(rxSubWin->UnReceiveNum != 0)
    {
        while(rxbegin != rxnext)
        {
            MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack index %d, SSN %d, receive status %d", rxbegin, rxSubWin->Win[rxbegin].SSN, rxSubWin->Win[rxbegin].status);
            rxbegin = SubWinPlus(rxbegin);
        }
    }
*/
#ifdef RETRANS_OPTIMIZE
    MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] MPD_SubWinCreateSack index %d begin %d, %d, %d, nxt %d, %d, %d", index, rxbegin, rxSubWin->Win[rxbegin].SSN, rxSubWin->Win[rxbegin].status, rxnext, rxSubWin->Win[rxnext].SSN, rxSubWin->Win[rxnext].status);
    MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] 0 %d, %d, 1 %d, %d, 2 %d, %d",  rxSubWin->Win[0].SSN, rxSubWin->Win[0].status, rxSubWin->Win[1].SSN, rxSubWin->Win[1].status, rxSubWin->Win[2].SSN, rxSubWin->Win[2].status);
	while(rxbegin != rxnext)
	{
		temp = SubWinPlus(rxbegin);
		status = rxSubWin->Win[temp].status;
		//MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack begin %d rxnext %d,ssn %d status %d, temp %d ssn%d status %d", rxSubWin->RxBegin,rxnext,rxSubWin->Win[rxSubWin->RxBegin].SSN, rxSubWin->Win[rxSubWin->RxBegin].status, temp, rxSubWin->Win[temp].SSN, status);
		if(rxSubWin->Win[rxbegin].status != status)
		{
			if(status == Received)
			{
				sack[sackNum].SackL[0] = rxSubWin->Win[temp].SSN>>8;
				sack[sackNum].SackL[1] = rxSubWin->Win[temp].SSN;
				MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack sack %d, SackL  %d", sackNum, rxSubWin->Win[temp].SSN);
			}
			else
			{
				sack[sackNum].SackR[0] = rxSubWin->Win[rxbegin].SSN>>8;
				sack[sackNum].SackR[1] = rxSubWin->Win[rxbegin].SSN;
				MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack sack %d, SackR  %d", sackNum, rxSubWin->Win[rxbegin].SSN);
				sackNum++;
				if(sackNum >= MAX_SACK_NUM)
				{
					MPD_SubWinSendSackPacketMsg(pUser, ack, (U8*)sack ,sackNum, index, firstSack);
					firstSack = 0;
					sackNum = 0;
					ack = rxSubWin->Win[SubWinPlus(rxbegin)].SSN;
					rxSubWin->SentAck = ack;
				}
			}
		}
		rxbegin = SubWinPlus(rxbegin);
	}
	//rxSubWin[index].NeedSendSack = false;
	if((sackNum > 0)||(firstSack != 0))
	    MPD_SubWinSendSackPacketMsg(pUser, ack, (U8*)sack ,sackNum, index, firstSack);
	rxSubWin->SentAck = rxSubWin->Win[rxSubWin->RxBegin].SSN;
#else
    while(rxSubWin->RxBegin != rxSubWin->RxNext)
    {
        temp = SubWinPlus(rxSubWin->RxBegin);
        status = rxSubWin->Win[temp].status;
        //MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack begin %d ssn %d status %d, temp %d ssn%d status %d", rxSubWin->RxBegin,rxSubWin->Win[rxSubWin->RxBegin].SSN, rxSubWin->Win[rxSubWin->RxBegin].status, temp, rxSubWin->Win[temp].SSN, status);
        if(rxSubWin->Win[rxSubWin->RxBegin].status != status)
        {
            if(status == Received)
            {
                sack[sackNum].SackL[0] = rxSubWin->Win[temp].SSN>>8;
                sack[sackNum].SackL[1] = rxSubWin->Win[temp].SSN;
                //MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack sack %d, SackL  %d", sackNum, rxSubWin->Win[temp].SSN);
            }
            else
            {
                sack[sackNum].SackR[0] = rxSubWin->Win[rxSubWin->RxBegin].SSN>>8;
                sack[sackNum].SackR[1] = rxSubWin->Win[rxSubWin->RxBegin].SSN;
                //MPDLOG(pUser->uid, LLV_INFO, "[MPDRX] SubWinCreateSack sack %d, SackR  %d", sackNum, rxSubWin->Win[rxSubWin->RxBegin].SSN);
                sackNum++;
                if(sackNum >= MAX_SACK_NUM)
                {
                    MPD_SubWinSendSackPacketMsg(pUser, ack, (U8*)sack ,sackNum, index);
                    sackNum = 0;
                    ack = rxSubWin->Win[SubWinPlus(rxSubWin->RxBegin)].SSN;
                    rxSubWin->SentAck = ack;
                }
            }
        }
        rxSubWin->RxBegin = SubWinPlus(rxSubWin->RxBegin);

    }

    //rxSubWin[index].NeedSendSack = false;
    if(sackNum > 0)
        MPD_SubWinSendSackPacketMsg(pUser, ack, (U8*)sack ,sackNum, index);
    rxSubWin->SentAck = rxSubWin->Win[rxSubWin->RxBegin].SSN;
#endif
}

void MPD_SackPeriodSend(stUserInfo* pUser)
{
    U8 i = 0;
    if(g_time == pUser->SendSackTime)
    {
        for(i=0;i < NUM_SUBFLOW; i++)
        {
            if(MPD_SCHD_getFlowValid(pUser, i))
            {
                MPD_SubWinCreateSack(pUser, i);
            }
        }
        pUser->SendSackTime = g_time + SackTimeOut;
    }
}    
   
U16 MPD_GetSubFlowAck(stUserInfo* pUser, U8 index)
{
    U16 ack; 
    ack = pUser->RxSubWin[index].Win[pUser->RxSubWin[index].RxBegin].SSN;
    pUser->RxSubWin[index].SentAck = ack;
    
    MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_GetSubFlowAck sfId %d, rxBegin  %d, ack %d", index, pUser->RxSubWin[index].RxBegin, ack);
    return ack;
}

void MPD_ClearSubWin(stUserInfo* pUser, S32 index)
{
    MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] ClearSubWin %d",index);
    //初始化窗口
    memset(&pUser->RxSubWin[index],0,sizeof(stRxSubWin));
}

void MPD_ClearDataWin(stUserInfo* pUser)
{
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] ClearDataWin rxBegin %d, rxNxt %d",pUser->RxDataWin.RxBegin, pUser->RxDataWin.RxNext);
    while(pUser->RxDataWin.RxBegin != pUser->RxDataWin.RxNext)
    {
        //将datawin清空
        MPD_DataWinRxBeginMove(pUser, true);
    }
    memset(&pUser->RxDataWin,0,sizeof(stRxDataWin));
}


void MPD_ChangeDataWinFlowId(stUserInfo* pUser, U8 flowId)
{
    MPDLOG(pUser->uid, LLV_WARNING, "[MPDRX] MPD_ChangeDataWinFlowId rxBegin %d, rxNxt %d",pUser->RxDataWin.RxBegin, pUser->RxDataWin.RxNext);
    U16 rxBegin = pUser->RxDataWin.RxBegin;
    U16 rxNext = pUser->RxDataWin.RxNext;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    
    while(rxBegin != rxNext)
    {
        if(rxDataWin->Win[rxBegin].status != UnReceived)
        {
            if(rxDataWin->Win[rxBegin].stream == flowId) 
                rxDataWin->Win[rxBegin].stream = NUM_SUBFLOW;  //这个流不存在
        }
        rxBegin = DataWinPlus(rxBegin);
    }
}

void MPD_CheckAck(stUserInfo* pUser)
{
    U8 mpDataOffset = 0;
    U16 i=0;
    stRxDataWin* rxDataWin = &pUser->RxDataWin;
    if(rxDataWin->NewData==false)
        return;
    for(i=rxDataWin->RxBegin;i!=rxDataWin->RxNext;i=DataWinPlus(i))
	{
		if(rxDataWin->Win[i].status != ReceiveComplete)
			continue;
		if(rxDataWin->Win[i].flag != UnChecked)
			continue;
		rxDataWin->Win[i].flag=Checked;
        mpDataOffset = IPUDPHeadOffset + rxDataWin->Win[i].mpdHeadLen;
        U8* dataPtr = MPD_GetMsgDataPtr(rxDataWin->Win[i].msgPtr[0],false);
        stIPHeader* ipHead = (stIPHeader*)(dataPtr+mpDataOffset);
		if(ipHead->proto!=6)
			continue;
		stTCPHeader* tcpHead = (stTCPHeader*)(dataPtr+mpDataOffset+ipHead->length*4);
        //U16 tcpseglen=((ipHead->len[0]<<8)+ipHead->len[1])-ipHead->length*4-tcpHead->length*4;
        //MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] MPD_CheckAck,tcpseglen=%d,ipHead->length=%d,tcpHead->length=%d",tcpseglen,ipHead->length,tcpHead->length);
		if(((ipHead->len[0]<<8)+ipHead->len[1])-ipHead->length*4-tcpHead->length*4!=0)
			continue;
		rxDataWin->Win[i].flag=Sended; 
		MPDLOG(pUser->uid, LLV_NOTIFY, "[MPDRX] MPD_CheckAck,index %d,dsn %d,Frag %d,addr %x", i, rxDataWin->Win[i].DSN, rxDataWin->Win[i].fragment,rxDataWin->Win[i].msgPtr[0]);
        MPD_PostPreMPDataToTransport(pUser->addressID, (const char*)rxDataWin->Win[i].msgPtr[0], rxDataWin->Win[i].msgLen[0], mpDataOffset);
	}
	rxDataWin->NewData=false;
}














