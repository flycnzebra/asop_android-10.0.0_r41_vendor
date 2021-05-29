/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_sack.c
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
#include "mpd_sack.h"
#include "mpd_tx.h"
#include <string.h>
/******************************************************************************/
#ifdef RETRANS_OPTIMIZE
#define SACK_TLV_BASE  4         //sack header 4 bytes: tag and length and RSV[2]
#else
#define SACK_TLV_BASE  2         //sack header 2 bytes: tag and length
#endif
#define SACK_TAG       0x01
#define INVALID_SACK   0xFFFF
/******************************************************************************/
extern void MPD_SCHD_RTTFlowUpdate(stUserInfo* pUser, U8 flowId, stDataNode* pNode);
/******************************************************************************/
void MPD_SACKInit(stUserInfo* pUser)
{
    memset(pUser->sackBlock, 0xff, sizeof(stSACKBlock)*MAX_SACK_NUM);
}

#ifdef RETRANS_OPTIMIZE
void MPD_processACK(stUserInfo* pUser, U8 sfId, U16 ackSSN, U8 firstSack)
#else
void MPD_processACK(stUserInfo* pUser, U8 sfId, U16 ackSSN)
#endif
{
    stTxWin* pTxWin = &pUser->txWindow[sfId];

    stDataNode* pTxBgn;
    //stDataNode* pTxEnd = (pTxWin->pTxNext-1);
#ifdef MAG
    //just for test
    pTxBgn = pTxWin->pTxBegin;
    while (pTxBgn != pTxWin->pTxNext)
    {
        if(pTxBgn->pData != pTxBgn->pData1)
        {
            MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error pData 0x%x, pData1 0x%x,SSN %d, sfId %d !", pTxBgn->pData, pTxBgn->pData1, pTxBgn->SSN, sfId);
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
#endif	

    if (sfId > NUM_SUBFLOW)   //=MAX_SACK_NUM)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_processACK(), subflowId %d error!", sfId);
        return;
    }
#ifdef RETRANS_OPTIMIZE
    if(!firstSack)
    {
        return;
    }
#endif
    pTxWin->ackTimeOutcnt = 0;
    //if ((DISTANCESN(pTxWin->pTxBegin->SSN, ackSSN) > DISTANCESN(pTxWin->pTxBegin->SSN, pTxEnd->SSN) + 1)
    if ((DISTANCESN(pTxWin->pTxBegin->SSN, ackSSN) > DISTANCESN(pTxWin->pTxBegin->SSN, pTxWin->pTxNext->SSN))
        ||(DISTANCESN(pTxWin->pTxBegin->SSN, ackSSN)==0))
    {
        //log: ackSSN out of window, either has been confirmed or error ackSSN
        MPDLOG(pUser->uid, LLV_NOTIFY, "sfId %d, receive ACKSSN %d out of window[%d, %d]", sfId, ackSSN, pTxWin->pTxBegin->SSN, pTxWin->pTxNext->SSN);
        return;
    }
    MPDLOG(pUser->uid, LLV_DEBUG, "receive ACKSSN %d subflowId %d during window [%d, %d]", ackSSN, sfId, pTxWin->pTxBegin->SSN, pTxWin->pTxNext->SSN);
    
    while (pTxWin->pTxBegin != pTxWin->pTxNext) //==, all dataNodes in window are confirmed
    {
        MPD_SCHD_RTTFlowUpdate(pUser,sfId,pTxWin->pTxBegin);
        //MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_processACK() flowId %d  begin %d g_time %d T1 %d", 
                                        //sfId, pTxWin->pTxBegin->SSN, g_time, pTxWin->pTxBegin->T1);

#ifdef TRAFFIC_CTL
		if(pTxWin->pTxBegin->pData)
			pUser->trafficControl[sfId].pktRate++;
#endif	
#ifdef SCHD_BALANCE_NEW 
        if(pTxWin->pTxBegin->pData)
            pUser->schdPara[sfId].PktCnt++;
#endif

        MPD_freeTxBeginData(pUser, sfId);
        MPD_txBeginInc(pUser, sfId);
#if 0 //def SCHD_BALANCE_NEW        
        pUser->schdPara[sfId].PktCnt++;
#endif

#if  0  //def TRAFFIC_CTL
		pUser->trafficControl[sfId].pktRate++;
#endif
        if (DISTANCESN(pTxWin->pTxBegin->SSN, ackSSN) == 0) //all data nodes before ackSSN are confirmed
            break;
    }

#ifdef TRAFFIC_CTL
	U16  endSsn = pUser->trafficControl[sfId].ssnEnd;
	if(DISTANCESN(endSsn, pTxWin->pTxNext->SSN) > DISTANCESN(pTxWin->pTxBegin->SSN, pTxWin->pTxNext->SSN))
	{
		pUser->trafficControl[sfId].ssnEnd = pTxWin->pTxBegin->SSN;
	}
#endif


//for test
#ifdef MAG
pTxBgn = pTxWin->pTxBegin;
while (pTxBgn != pTxWin->pTxNext)
{
    if(pTxBgn->pData != pTxBgn->pData1)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error pData 0x%x, pData1 0x%x,SSN %d, sfId %d !", pTxBgn->pData, pTxBgn->pData1, pTxBgn->SSN, sfId);
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
#endif	
}
#ifdef RETRANS_OPTIMIZE
void MPD_parserSACK(stUserInfo* pUser, U8 sfId, U8* pSack, U16 ack)
#else
void MPD_parserSACK(stUserInfo* pUser, U8 sfId, U8* pSack)
#endif
{
    U8 i, sackIdx;
    U8 tag;
    U8 length;
    U8 numSack;
    U8 leftInWin;
    U8 rightInWin;
    stTxWin* pTxWin;
    stSACKBlock* pBlk;
    U16 leftEdgeSSN;
    U16 rightEdgeSSN;
#ifdef RETRANS_OPTIMIZE
	U8 firstSack;	
    stDataNode* pTxBegin;       
    stDataNode* pTxNext;        
#endif
    if (sfId>=NUM_SUBFLOW)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error flowId %d", sfId);
        return;
    }
    if (pSack == NULL)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error SACK ptr!");
        return;
    }


#ifdef TRAFFIC_CTL
	pUser->trafficControl[sfId].sackNum++;
#endif

    pTxWin = &pUser->txWindow[sfId];
    tag = *pSack;
    length = *(pSack + 1);
#ifdef RETRANS_OPTIMIZE
	firstSack = *(pSack + 2);
#endif
    numSack = (length - SACK_TLV_BASE)>>2; //each sack block is 4 bytes

    if (tag != SACK_TAG)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), error tag in SACK header!");
        return;
    }
    if (numSack > MAX_SACK_NUM)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), numSack %d overflow! length %d", numSack, length);
        return;
    }

    if (numSack == 0)
    {
        MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(), numSack %d !tag %d, length %d", numSack,tag, length);
        return;
    }
    MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK(),sfId %d, numSack %d ! tag %d, length %d",sfId, numSack,tag, length);

#ifdef SCHD_BALANCE
    pUser->txWindow[sfId].retransCnt = 0;
#endif
    pBlk = (stSACKBlock*)(pSack+4);

    MPD_SACKInit(pUser);
    for (i=0; i<numSack; i++)
    {
        pUser->sackBlock[i].leftEdgeSSN[0]  = pBlk->leftEdgeSSN[0];
        pUser->sackBlock[i].leftEdgeSSN[1]  = pBlk->leftEdgeSSN[1];
        pUser->sackBlock[i].rightEdgeSSN[0] = pBlk->rightEdgeSSN[0];
        pUser->sackBlock[i].rightEdgeSSN[1] = pBlk->rightEdgeSSN[1];
        pBlk++;
    }

    sackIdx = 0;
#ifdef SACK_OPTIMIZE 
#ifdef RETRANS_OPTIMIZE
        if(!firstSack)
        {
            pTxBegin = &pTxWin->dataNode[ack%MAX_TX_WINDOW_SIZE];
            if(pTxBegin->SSN != ack)
            {
                MPDLOG(pUser->uid, LLV_ERROR, "error ack not find %d ", ack);
                return;
            }
        }
        else
        {
    		pTxBegin = pTxWin->pTxBegin;
        }
		pTxNext = pTxWin->pTxNext;		


        
    //MYLOG(0xffff, LLV_WARNING,"MPD_parserSACK(),sfId %d, numSack %d ! begin %d, nxt %d",sfId, numSack,pTxBegin->SSN, pTxNext->SSN);
        for (sackIdx=0; sackIdx<numSack; sackIdx++)
        {
            leftEdgeSSN = (pUser->sackBlock[sackIdx].leftEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].leftEdgeSSN[1];
            rightEdgeSSN = (pUser->sackBlock[sackIdx].rightEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].rightEdgeSSN[1];
            MPDLOG(pUser->uid, LLV_NOTIFY, "sfId %d,sackIdx %d: txBeginSSN %5d txEndSSN %5d leftSSN %5d rightSSN %5d",
                sfId,sackIdx, pTxBegin->SSN, pTxNext->SSN, leftEdgeSSN, rightEdgeSSN);
            
            //if (leftEdgeSSN == INVALID_SACK)
                //break;
            //leftInWin  = MPD_isSSNInTxWin(pTxWin, leftEdgeSSN);
            //rightInWin = MPD_isSSNInTxWin(pTxWin, rightEdgeSSN);
            MPDLOG(pUser->uid, LLV_NOTIFY, "leftInWindow %d rightInWindow %d", leftInWin, rightInWin);
            
            if (DISTANCESN(pTxBegin->SSN, leftEdgeSSN) < DISTANCESN(pTxBegin->SSN, pTxNext->SSN))  //    //(pTxNext-1)->SSN))  
            {
                while(DISTANCESN(pTxBegin->SSN, leftEdgeSSN) != 0)
                {
                    MPDLOG(pUser->uid, LLV_DEBUG, "reTrans DataSSN %d", pTxBegin->SSN);

#ifdef TRAFFIC_CTL
					if(pTxBegin->transState == WAITING_ACK)
					{
						pUser->trafficControl[sfId].pktLossCnt++;
					}
#endif

                    if((pTxBegin->transState == WAITING_ACK) || 
                       ((pTxBegin->transState == SACKED) && (g_time >= pTxBegin->TO_retrans)))
                    {
                        MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_parserSACK&MPD_callMPCToRetrans sfId %d, ssn %d, state %d, retransCnt %d", 
                            sfId,pTxBegin->SSN,pTxBegin->transState,pUser->txWindow[sfId].retransCnt);
                        MPD_callMPCToRetrans(pUser, sfId, pTxBegin);
                        pTxBegin->transState = SACKED;
                        //MYLOG(0xffff, LLV_WARNING,"MPD_parserSACK, 11 flowId %d !", sfId);
                        if(sfId == 0) //McWill
                            pTxBegin->TO_retrans = g_time + RETRANS_TO_MC;  //10´ý¶¨
                        else    
                            pTxBegin->TO_retrans = g_time + RETRANS_TO_OTHER;  //
#ifdef SCHD_BALANCE
                        pUser->txWindow[sfId].retransCnt++;
#endif                        

#ifdef SCHD_BALANCE_NEW        
                        pUser->schdPara[sfId].pktRetCnt++;
#endif

                    }
                    
                    if(pTxBegin ==  &pTxWin->dataNode[MAX_TX_WINDOW_SIZE-1])
				    {
				        pTxBegin = &pTxWin->dataNode[0];
				    }
				    else
				    {
				        pTxBegin++;
				    }
				    MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_txBeginInc() ptxBegin-->DataNode[%d]", (pTxBegin - (&pTxWin->dataNode[0])));
                }

                if(DISTANCESN(pTxBegin->SSN, rightEdgeSSN) < DISTANCESN(pTxBegin->SSN, pTxNext->SSN))
                {
                    //while(DISTANCESN(pTxBegin->SSN, rightEdgeSSN+1) != 0)
                    while(DISTANCESN(pTxBegin->SSN, rightEdgeSSN) < RCV_SN_RANGE)
                    {
                        MPDLOG(pUser->uid, LLV_NOTIFY, "txBegin dataSSN %d sacked", pTxBegin->SSN);
                        if(pTxBegin->transState != DATA_INIT && NULL != pTxBegin->pData)
                        {
                            MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_parserSACK() MPD_freeTxBeginData flowId %d nodeSSN %d, begin %d, T1 %d free memory", 
                                sfId, pTxBegin->SSN, pTxWin->pTxBegin->SSN, pTxWin->pTxBegin->T1);
#ifdef SCHD_DELAY
							MPD_SCHD_RTTFlowUpdate(pUser,sfId,pTxBegin);
#endif
                            if(pTxBegin->transState == SACKED)
                            {
                                //MYLOG(0xffff, LLV_WARNING,"MPD_parserSACK, Free_ResendBuf, flowId %d, ssn %d !", sfId, pTxBegin->SSN);
#if 0 //def MAG
MPDLOG(pUser->uid, LLV_ERROR, "MPD_RetransMsgMemfree(), flowId %d,data is 0x%x", sfId, pTxBegin->pData);
#endif
//just for test
if((pTxBegin->magic1 != 0x5a5a5a5a) || (pTxBegin->magic2 != 0x5a5a5a5a))
{
    MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK() magicError sfId %d nodeSSN %d magic1 0x%x magic2 0x%x", 
        sfId, pTxBegin->SSN, pTxBegin->magic1, pTxBegin->magic2);
}

                                MPD_RetransMsgMemfree(pTxBegin->pData,sfId);
                            }
                            else
                            {
                                //MYLOG(0xffff, LLV_WARNING,"MPD_parserSACK, MPD_MsgMemfree, pTxBegin->transState %d !", pTxBegin->transState);
                                MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", pTxBegin->pData);
#if 0  //def MAG
MPDLOG(pUser->uid, LLV_ERROR, "MPD_MsgMemfree(), flowId %d,data is 0x%x", sfId, pTxBegin->pData);
#endif
//just for test
if((pTxBegin->magic1 != 0x5a5a5a5a) || (pTxBegin->magic2 != 0x5a5a5a5a))
{
    MPDLOG(pUser->uid, LLV_ERROR, "MPD_parserSACK() magicError sfId %d nodeSSN %d magic1 0x%x magic2 0x%x", 
        sfId, pTxBegin->SSN, pTxBegin->magic1, pTxBegin->magic2);
}

                                MPD_MsgMemfree(pTxBegin->pData);
                            }
                            pTxBegin->pData = NULL;
                            pTxBegin->pData1 = NULL;
                            pTxBegin->transState = DATA_INIT;
#ifdef SCHD_BALANCE_NEW        
                            pUser->schdPara[sfId].PktCnt++;
#endif
#ifdef TRAFFIC_CTL
							pUser->trafficControl[sfId].pktRate++;
#endif

                        }
                        
                        if(pTxBegin ==  &pTxWin->dataNode[MAX_TX_WINDOW_SIZE-1])
                        {
                            pTxBegin = &pTxWin->dataNode[0];
                        }
                        else
                        {
                            pTxBegin++;
                        }
                    }
                }
                else
                {
                    MPDLOG(pUser->uid, LLV_NOTIFY, "error,beginSsn %d, rightEdgeSSN %d, nextSsn %d,move to next sackBlock", 
                        pTxBegin->SSN, rightEdgeSSN, pTxNext->SSN);
                    //sackIdx++;
                }
            }
            else
            {
                MPDLOG(pUser->uid, LLV_ERROR, "error,beginSsn %d, leftEdgeSSN %d, nextSsn %d,move to next sackBlock", 
                        pTxBegin->SSN, leftEdgeSSN, pTxNext->SSN);
                //sackIdx++;
            }
        }	

#ifdef TRAFFIC_CTL
		U16  endSsn = pUser->trafficControl[sfId].ssnEnd;
		if(DISTANCESN(endSsn, pTxNext->SSN) > DISTANCESN(pTxBegin->SSN, pTxNext->SSN))
		{
			pUser->trafficControl[sfId].ssnEnd = pTxBegin->SSN;
		}
#endif
#else
        for (sackIdx=0; sackIdx<numSack; sackIdx++)
        {
            leftEdgeSSN = (pUser->sackBlock[sackIdx].leftEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].leftEdgeSSN[1];
            rightEdgeSSN = (pUser->sackBlock[sackIdx].rightEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].rightEdgeSSN[1];
            MPDLOG(pUser->uid, LLV_DEBUG, "sackIdx %d: txBeginSSN %5d txEndSSN %5d leftSSN %5d rightSSN %5d",
                sackIdx, pTxWin->pTxBegin->SSN, (pTxWin->pTxNext-1)->SSN, leftEdgeSSN, rightEdgeSSN);
            
            //if (leftEdgeSSN == INVALID_SACK)
                //break;
            leftInWin  = MPD_isSSNInTxWin(pTxWin, leftEdgeSSN);
            rightInWin = MPD_isSSNInTxWin(pTxWin, rightEdgeSSN);
            MPDLOG(pUser->uid, LLV_DEBUG, "leftInWindow %d rightInWindow %d", leftInWin, rightInWin);
            
            if (leftInWin)
            {
                while(DISTANCESN(pTxWin->pTxBegin->SSN, leftEdgeSSN) != 0)
                {
                    MPDLOG(pUser->uid, LLV_DEBUG, "reTrans DataSSN %d", pTxWin->pTxBegin->SSN);
                    MPD_callMPCToRetrans(pUser, sfId, pTxWin->pTxBegin);
#ifdef SCHD_BALANCE
                    pUser->txWindow[sfId].retransCnt++;
#endif
#ifdef SCHD_BALANCE_NEW        
                    pUser->schdPara[sfId].pktRetCnt++;
#endif

                    MPD_txBeginInc(pUser, sfId);
                }
            }
            else if(rightInWin)
            {
                while(DISTANCESN(pTxWin->pTxBegin->SSN, rightEdgeSSN+1) != 0)
                {
                    MPDLOG(pUser->uid, LLV_DEBUG, "txBegin dataSSN %d sacked, free memory", pTxWin->pTxBegin->SSN);
                    MPD_SCHD_RTTFlowUpdate(pUser,sfId,pTxWin->pTxBegin);
                    MPD_freeTxBeginData(pUser, sfId);
                    MPD_txBeginInc(pUser, sfId);
#ifdef SCHD_BALANCE_NEW        
                    pUser->schdPara[sfId].PktCnt++;
#endif
#ifdef TRAFFIC_CTL
					pUser->trafficControl[sfId].pktRate++;
#endif

                }
            }
            else
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "move to next sackBlock");
                sackIdx++;
            }
        }
#endif
#else
    while(pTxWin->pTxBegin != pTxWin->pTxNext)
    {
        leftEdgeSSN = (pUser->sackBlock[sackIdx].leftEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].leftEdgeSSN[1];
        rightEdgeSSN = (pUser->sackBlock[sackIdx].rightEdgeSSN[0]<<8)+pUser->sackBlock[sackIdx].rightEdgeSSN[1];
        MPDLOG(pUser->uid, LLV_DEBUG, "sackIdx %d: txBeginSSN %5d txEndSSN %5d leftSSN %5d rightSSN %5d",
            sackIdx, pTxWin->pTxBegin->SSN, (pTxWin->pTxNext-1)->SSN, leftEdgeSSN, rightEdgeSSN);

        //if (leftEdgeSSN == INVALID_SACK)
            //break;
        leftInWin  = MPD_isSSNInTxWin(pTxWin, leftEdgeSSN);
        rightInWin = MPD_isSSNInTxWin(pTxWin, rightEdgeSSN);
        MPDLOG(pUser->uid, LLV_DEBUG, "leftInWindow %d rightInWindow %d", leftInWin, rightInWin);

        if (!leftInWin && !rightInWin)
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "move to next sackBlock");
            sackIdx++;
        }
        else if (leftInWin && rightInWin && DISTANCESN(pTxWin->pTxBegin->SSN, leftEdgeSSN) != 0)
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "reTrans DataSSN %d", pTxWin->pTxBegin->SSN);
            MPD_callMPCToRetrans(pUser, sfId, pTxWin->pTxBegin);
#ifdef SCHD_BALANCE
            pUser->txWindow[sfId].retransCnt++;
#endif
#ifdef SCHD_BALANCE_NEW        
            pUser->schdPara[sfId].pktRetCnt++;
#endif

            MPD_txBeginInc(pUser, sfId);
        }
        else
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "txBegin dataSSN %d sacked, free memory", pTxWin->pTxBegin->SSN);
            MPD_SCHD_RTTFlowUpdate(pUser,sfId,pTxWin->pTxBegin);
            MPD_freeTxBeginData(pUser, sfId);
            MPD_txBeginInc(pUser, sfId);
        }
    }
#endif
}
