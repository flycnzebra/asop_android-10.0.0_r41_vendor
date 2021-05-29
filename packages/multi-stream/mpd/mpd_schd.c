/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_schd.c
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

/******************************************************************************/
#define max(a,b)             (((a)>(b))?(a):(b))
#define min(a,b)             (((a)<(b))?(a):(b))
#define Abs(a,b)             (((a)>(b))?(a-b):(b-a))

/******************************************************************************/
extern U32 g_time;
stSchdCfg  g_schdCfg;

//int g_isTrafficCtl = 1;
#ifdef MAG
extern int g_isTrafficCtl; //0,off, 1,on
extern int g_mpdMcDelay;  //cwy added
extern int g_InhertDelay;  //the mcwill delay
extern int g_isMdelayMin;
extern int g_MminusPkt;
#else
int g_isTrafficCtl = 0; //0,off, 1,on
int g_mpdMcDelay = 6;  //cwy added
int g_InhertDelay = 50;  //the mcwill delay
int g_isMdelayMin = 1;
int g_MminusPkt = 0;
#endif
extern bool MPD_isTxWinEmpty(stUserInfo* pUser, U8 flowId);
U32 MPD_getTxWinUnackCnt(stUserInfo* pUser, U8 flowId);
#ifndef MAG
extern RAT_Env g_mpcFun;
#endif
#ifdef SCHD_NEW
U16 Schdtable[TABLE_SIZE];
#endif
#if 0 //def SCHD_DELAY
extern U32 MPD_getHistAcnt(stUserInfo* pUser, U8 flowId);
#endif
#ifdef SCHD_BALANCE
extern U32 MPD_getTxWinRetransCnt(stUserInfo* pUser, U8 flowId);
#endif
/******************************************************************************/

void MPD_SCHD_Init(stUserInfo* pUser)
{
    U8 id = 0;
    pUser->RTTFlowMax = 0;
    for (id=0; id<NUM_SUBFLOW; id++)
    {
        pUser->schdPara[id].isValid = 0;
        pUser->schdPara[id].RTTNew  = 0;
        pUser->schdPara[id].RTTFlow = 0;
#ifdef FIXED_NWT 
        pUser->schdPara[id].Nweight = 1;
#else
        pUser->schdPara[id].Nweight = g_schdCfg.NN>>1;
#endif
        pUser->schdPara[id].wCnt    = g_schdCfg.NN;
#ifdef SCHD_NEW
        pUser->schdPara[id].RTTDev = 0;
        pUser->schdPara[id].RTO = 10;
#endif
#ifdef SCHD_DELAY
		pUser->schdPara[id].InhertDelay = 5; //0;
		pUser->schdPara[id].isRttUpdate = false;
        pUser->schdPara[id].isPeriodRttUpdate = true;
#endif
#ifdef SCHD_BALANCE_NEW        
        pUser->schdPara[id].ModifyFactor = 100;
        pUser->schdPara[id].PktAvg = 128;
        //pUser->schdPara[id].ThCnt = 0;
#else
        pUser->schdPara[id].ModifyFactor = 1;
        pUser->schdPara[id].PktAvg = 0;

#endif
#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
        pUser->schdPara[id].PktCnt = 0;
        pUser->schdPara[id].pktRetCnt = 0;
        pUser->schdPara[id].PktChooseCnt = 0;
#endif        
    }
}

void MPD_SCHD_CfgInit()
{
    g_schdCfg.scaleRTTFlow                  = 32;
#ifdef SCHD_NEW    
    g_schdCfg.scaleRTTDev                   = 4;
    g_schdCfg.scaleRTO0                     = 1;
    g_schdCfg.scaleRTO1                     = 4;
    g_schdCfg.linkThrptStatisPeriod         =1;
    g_schdCfg.updateAcklatencyPeriod        =1;
    g_schdCfg.updateModifyFactorPeriod      =1;
#endif
#ifdef SCHD_BALANCE_NEW
    g_schdCfg.scalePktV                     = 5;
#endif
#ifdef SCHD_NEW  
    g_schdCfg.scalePktV                     = 20;
#endif
    g_schdCfg.weightUpdatePeriod            = 200;
    g_schdCfg.NN                            = 2048;
    g_schdCfg.wCntOverflowTh                = 65535;
}

void MPD_SCHD_setAlgVar(U8 alpha, U16 period, U32 step, U32 cntTh)
{
    g_schdCfg.scaleRTTFlow         = alpha;
    g_schdCfg.weightUpdatePeriod = period;
    g_schdCfg.NN                 = step;
    g_schdCfg.wCntOverflowTh     = cntTh;
}

void MPD_SCHD_setFlowValid(stUserInfo* pUser, U32 flowId, U32 flag)
{
    pUser->schdPara[flowId].isValid = (U8)flag;
}
U8 MPD_SCHD_getFlowValid(stUserInfo* pUser, U8 flowId)
{
    return pUser->schdPara[flowId].isValid;
}
void MPD_SCHD_RTTFlowUpdate(stUserInfo* pUser, U8 flowId, stDataNode* pNode)
{
#ifdef SCHD_DELAY
	if(pNode->transState == WAITING_ACK)
	{
		pUser->schdPara[flowId].RTTNew  = g_time - pNode->T1;
		pUser->schdPara[flowId].isRttUpdate = true;
	}
#endif

#if (defined FIXED_NWT || defined SCHD_BALANCE) 
    return;
#endif
    //RTT calculate
    //((rtt_flow<<ALPHA_SCALE - rtt_flow) + rtt_new)>>ALPHA_SCALE
    pUser->schdPara[flowId].RTTNew  = g_time - pNode->T1;
    pUser->schdPara[flowId].RTTFlow = ((pUser->schdPara[flowId].RTTFlow * (g_schdCfg.scaleRTTFlow - 1) + pUser->schdPara[flowId].RTTNew)/g_schdCfg.scaleRTTFlow);
    pUser->RTTFlowMax = max(pUser->RTTFlowMax, pUser->schdPara[flowId].RTTFlow);
}

#ifdef SCHD_DELAY
void MPD_SCHD_DelayUpdate(stUserInfo* pUser) 
{
	U32  rtt = 0;
	U8 	 flowId = 0;
	U32  inhertDelayOld = pUser->schdPara[flowId].InhertDelay;
	
	if((g_time % T_PKT) == (pUser->uid % T_PKT))
	{
		for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
		{
            if (!pUser->schdPara[flowId].isValid) 
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "MPD_SCHD_DelayUpdate：flowId %d: invalid flow", flowId);
                continue;
            }

			if(pUser->schdPara[flowId].isRttUpdate == false)
			{
				continue;
			}

            pUser->schdPara[flowId].isRttUpdate = false;
            pUser->schdPara[flowId].isPeriodRttUpdate = true;
            
			rtt = pUser->schdPara[flowId].RTTNew*10;
			if(pUser->schdPara[flowId].InhertDelay == 0)
			{
				pUser->schdPara[flowId].InhertDelay = rtt;
				continue;
			}
            
#if 0
            //just for test
            if(flowId == 0)
            {
                pUser->schdPara[flowId].InhertDelay = g_InhertDelay; // 50;
                continue;
            }
#endif
            
			if(pUser->schdPara[flowId].InhertDelay <= rtt) //慢升
			{
				//pUser->schdPara[flowId].InhertDelay = (pUser->schdPara[flowId].InhertDelay * 255 + rtt)/256;
				pUser->schdPara[flowId].InhertDelay = (pUser->schdPara[flowId].InhertDelay * 9 + rtt)/10;
			}
			else
			{
				pUser->schdPara[flowId].InhertDelay = (pUser->schdPara[flowId].InhertDelay + rtt)/2;
			}

            if(pUser->schdPara[flowId].InhertDelay < 5)
                pUser->schdPara[flowId].InhertDelay = 5;
            
			MPDLOG(pUser->uid, LLV_DEBUG, "MPD_SCHD_DelayUpdate：flowId %d:rttNew %d inhertDelayOld %d, InhertDelay %d", 
				flowId, rtt, inhertDelayOld, pUser->schdPara[flowId].InhertDelay);
		}
	}
}
#endif

#if 0
void MPD_SCHD_setPktSend(stUserInfo* pUser, U32 flowId, U32 flag)
{
    pUser->schdPara[flowId].isSchedValid = (U8)flag;
}
U8 MPD_SCHD_getPktSend(stUserInfo* pUser, U8 flowId)
{
    return pUser->schdPara[flowId].isSchedValid;
}
#endif

#ifdef SCHD_NEW
void MPD_SCHD_RTTDevUpdate(stUserInfo* pUser, U8 flowId)   
{
    U32 RTTDev = Abs(pUser->schdPara[flowId].RTTNew, pUser->schdPara[flowId].RTTFlow);
    pUser->schdPara[flowId].RTTDev = (pUser->schdPara[flowId].RTTDev * (g_schdCfg.scaleRTTDev - 1) + RTTDev )/g_schdCfg.scaleRTTDev;
}

void MPD_SCHD_RTOUpdate(stUserInfo* pUser, U8 flowId)   
{
    pUser->schdPara[flowId].RTO = pUser->schdPara[flowId].RTTFlow * g_schdCfg.scaleRTO0 + pUser->schdPara[flowId].RTTDev * g_schdCfg.scaleRTO1;
}

void MPD_SCHD_ModifyFactorUpdate(stUserInfo* pUser)
{
    U8 i = 0;
    U32 ackLatency[NUM_SUBFLOW] = {0};
    U32 ackLatencyTotal = 0;
    U32 ackLatencyAvg = 0;
    for(i=0; i<NUM_SUBFLOW; i++)
    {
        if(pUser->schdPara[i].PktAvg != 0)
        {
            ackLatency[i] = MPD_getTxWinUnackCnt(pUser, i)/ pUser->schdPara[i].PktAvg;
        }
        ackLatencyTotal += ackLatency[i];
    }
    ackLatencyAvg = ackLatencyTotal/NUM_SUBFLOW;
    if(ackLatencyAvg != 0)
    {
        for(i=0; i<NUM_SUBFLOW; i++)
        {
            pUser->schdPara[i].ModifyFactor = (1+(ackLatency[i]/ackLatencyAvg)) * g_schdCfg.scaleModifyFacktor * pUser->schdPara[i].Nweight;
        }
    }
}
#endif

#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
void MPD_SCHD_PktAvgUpdate(stUserInfo* pUser)
{
    U32 pktTmp;
    U32 pktAvg;
    U8  flowId = 0;
    
    if((g_time % T_PKT) == (pUser->uid % T_PKT))
    {
    
        for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
        {
            if (!pUser->schdPara[flowId].isValid)  // || !pUser->schdPara[flowId].isSendPkt)
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "MPD_SCHD_PktAvgUpdate：flowId %d: invalid flow", flowId);
                continue;
            }

			if(pUser->schdPara[flowId].PktChooseCnt == 0)
			{
				MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: PktChooseCnt is 0, pktCnt %d", flowId, pUser->schdPara[flowId].PktCnt);
			}
			
            pktTmp = pUser->schdPara[flowId].PktCnt * 100/T_PKT;
            pktAvg = (pUser->schdPara[flowId].PktAvg * (g_schdCfg.scalePktV-2) + 2*pktTmp)/g_schdCfg.scalePktV;
#if  1
			if(pUser->schdPara[flowId].txWinEmptyCnt > 0)
#else
            if(MPD_isTxWinEmpty(pUser, flowId))
#endif				
            {
                if(pUser->schdPara[flowId].PktAvg < pktAvg)  //发送窗为空时，不降
                    pUser->schdPara[flowId].PktAvg = pktAvg;
                //pUser->schdPara[flowId].PktAvg = max(pUser->schdPara[flowId].PktAvg*(g_schdCfg.scalePktV+1)/g_schdCfg.scalePktV, pktAvg);
            }
            else
            {
                pUser->schdPara[flowId].PktAvg = pktAvg;
            }
 
//for test
U32 uackCnt = MPD_getTxWinUnackCnt(pUser, flowId);
MPDLOG(pUser->uid, LLV_INFO, "flowId %d: PktCnt %d, pktRetCnt %d, PktChooseCnt %d, uackCnt %d, txWinEmptyCnt %d",flowId ,
	pUser->schdPara[flowId].PktCnt,pUser->schdPara[flowId].pktRetCnt,pUser->schdPara[flowId].PktChooseCnt, uackCnt, pUser->schdPara[flowId].txWinEmptyCnt);

            
#ifdef SCHD_BALANCE_NEW
            //if(flowId == 0)
            {
                if(pUser->schdPara[flowId].PktAvg< 38)    //  128)
                {
                    //pUser->schdPara[flowId].ThCnt++;
                    //if(pUser->schdPara[flowId].ThCnt > 5)
                    //{
                        pUser->schdPara[flowId].PktAvg = max(pUser->schdPara[flowId].PktAvg*3/2,38);  //  128);//限制M网测速的最低速度为每秒128个包
                        //pUser->schdPara[flowId].ThCnt = 0;
                    //}
                }
                //else
                //{
               //     pUser->schdPara[flowId].ThCnt = 0;
                //}
            }
            if(pUser->schdPara[flowId].PktAvg != 0)
            {
                U32 pktAvgTmp = 0;
#if	1
				if(pUser->schdPara[flowId].txWinEmptyCnt > 0)
#else
                if(MPD_isTxWinEmpty(pUser, flowId))
#endif					
                {
                    pktAvgTmp = pUser->schdPara[flowId].PktAvg*(g_schdCfg.scalePktV+1)/g_schdCfg.scalePktV;
                }
                else
                {
                    pktAvgTmp = pUser->schdPara[flowId].PktAvg;
                }

                pUser->schdPara[flowId].ModifyFactor = 10000/pktAvgTmp;            
                //pUser->schdPara[flowId].ModifyFactor = 10000/pUser->schdPara[flowId].PktAvg;
            }
            //pUser->schdPara[flowId].ModifyFactor = 1;//测试代码，一比一
#endif
            MPDLOG(pUser->uid, LLV_INFO, "flowId %d: schdParaPktAvg %d, ModifyFactor %d,pktTmp %d, pktAvg %d",flowId ,pUser->schdPara[flowId].PktAvg,  pUser->schdPara[flowId].ModifyFactor, pktTmp,  pktAvg);
            pUser->schdPara[flowId].PktCnt = 0;
			pUser->schdPara[flowId].pktRetCnt = 0;
			pUser->schdPara[flowId].PktChooseCnt = 0;
#if	1
			pUser->schdPara[flowId].txWinEmptyCnt = 0;
#endif
        }
    }
}
#endif

void MPD_SCHD_WeightUpdate(stUserInfo* pUser)
{
    U8 flowId;
#ifdef FIXED_NWT
    pUser->schdPara[flowId].Nweight = 1;
#else
    if((g_time % g_schdCfg.weightUpdatePeriod) == (pUser->uid % g_schdCfg.weightUpdatePeriod))
    {
        for(flowId = 0; flowId<NUM_SUBFLOW; flowId++)
        {
            if (pUser->RTTFlowMax == 0 || pUser->schdPara[flowId].RTTFlow ==0)
                continue;
            pUser->schdPara[flowId].Nweight = g_schdCfg.NN * pUser->schdPara[flowId].RTTFlow / pUser->RTTFlowMax;
        }
    }
#endif    
}


//从MPC获取各个流的RTT，来更新MPD的RTT
void MPD_SCHD_RttUpdate(stUserInfo* pUser)
{
    S32 flowId = 0;
    U32 rtt = 0;
    S32 netType = 0;
    
    if((g_time % 150) == (pUser->uid % 150))  //  1s更新一次各个流的RTT
    {
		for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
		{
			if (!pUser->schdPara[flowId].isValid)
            {
                //MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
                continue;
            }

            if (pUser->schdPara[flowId].isPeriodRttUpdate)
            {
                pUser->schdPara[flowId].isPeriodRttUpdate = false;
                continue;
            }
            else
            {
                netType = MPD_GetDeviceId(flowId);
#ifdef MAG     
                rtt = GetRTTC(pUser->addressID,netType);
#else
                rtt = (*g_mpcFun.GetRTTC)(netType);
#endif

                if(rtt > 0)
                    pUser->schdPara[flowId].InhertDelay = rtt;              //(*g_mpcFun.GetRTTC)(flowId);  //流，单位?
#ifdef MAG
                MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: netType %d,MPD_SCHD_RttUpdate, rtt %d", flowId, netType, pUser->schdPara[flowId].InhertDelay);
#else
                //MYLOGP(0xffff, LLV_DEBUG, "flowId %d: netType %d,MPD_SCHD_RttUpdate, rtt %d", flowId, netType, pUser->schdPara[flowId].InhertDelay);
#endif
            }            
        }
    }
}

#if	1
void MPD_CHECK_Txwin_Empty(stUserInfo* pUser)
{
	U8  flowId = 0;

	if((g_time % 10) == (pUser->uid % 10)) //100ms检查一次发送窗里是否为空
	{
		for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
		{
			if (!pUser->schdPara[flowId].isValid)
            {
                //MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
                continue;
            }

			if(MPD_isTxWinEmpty(pUser, flowId))
			{
				pUser->schdPara[flowId].txWinEmptyCnt++;
			}
		}
	}
}
#endif

#ifdef TRAFFIC_CTL
void MPD_TrafficCtlInit(stUserInfo* pUser)
{
	U8 flowId = 0;

	for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
	{
#if 0	
		if (!pUser->schdPara[flowId].isValid)
		{
			//MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
			continue;
		}
#endif

		pUser->trafficControl[flowId].pktRate = 128;  //默认发包速率100
		pUser->trafficControl[flowId].pktRatePre = 0;
		pUser->trafficControl[flowId].pktCntTx = 0;
		pUser->trafficControl[flowId].scaleFactor = 120;
		pUser->trafficControl[flowId].lossThrsh = 5;  //丢包率，默认
		pUser->trafficControl[flowId].pktRateAvg = 128;
		pUser->trafficControl[flowId].throuputLimit = 100;
		pUser->trafficControl[flowId].pktLossCnt = 0;
		pUser->trafficControl[flowId].ssnBegin = 0;
		pUser->trafficControl[flowId].ssnEnd = 0;
		pUser->trafficControl[flowId].isTrafficCtl = false;
		pUser->trafficControl[flowId].sackNum = 0;
        pUser->trafficControl[flowId].acceleratedCnt = 0;
		pUser->trafficControl[flowId].acceleratedPkt = 0;
		pUser->trafficControl[flowId].isPktTxStop = false;	
        pUser->trafficControl[flowId].quitTrafficCtlCnt = 0;
	}
	
	memset(pUser->dataWaitQueue, 0, sizeof(stWaitSendNode)*DataWaitQueueSize);
	pUser->pDataWaiteHead = 0;
	pUser->pDataWaiteTail = 0;
	pUser->throuputTotal = 500;
	pUser->isAllFlowTrafficCtl = false;
	pUser->isMpStopSched = false;
	pUser->tcDelPktCnt = 0;
}

void MPD_TrafficControl(stUserInfo* pUser)   
{
	U8 flowId;
    U32 rateAvg;
	U32 lossPktCnt;
	U32 sendPktCnt;
	//U32 sendPktCntAll = 0;
	//1秒钟周期


    if(g_isTrafficCtl == 0)
    {
        pUser->isAllFlowTrafficCtl = false;
		pUser->throuputTotal = 0;
		pUser->isMpStopSched = false;  
        for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
        {
			pUser->trafficControl[flowId].isTrafficCtl = false;
			pUser->trafficControl[flowId].isPktTxStop = false;
        }
        return;
    }
    
	if((g_time % T_TC_PERIOD) == (pUser->uid % T_TC_PERIOD))
	{
		pUser->isAllFlowTrafficCtl = true;
		pUser->throuputTotal = 0;
		pUser->isMpStopSched = false;
			
        for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
        {
            if (!pUser->schdPara[flowId].isValid)  // || !pUser->schdPara[flowId].isSendPkt)
            {
                MPDLOG(pUser->uid, LLV_DEBUG, "MPD_TrafficControl：flowId %d: invalid flow", flowId);
                continue;
            }
            
#if 0
			if(pUser->trafficControl[flowId].sackNum)
			{
				pUser->trafficControl[flowId].scaleFactor = pUser->trafficControl[flowId].scaleFactor*95/100;
			}
			else
			{
				pUser->trafficControl[flowId].scaleFactor = pUser->trafficControl[flowId].scaleFactor*105/100;
			}

			//pUser->trafficControl[flowId].scaleFactor出现到0的情况
			if(pUser->trafficControl[flowId].scaleFactor < 60)
				pUser->trafficControl[flowId].scaleFactor = 60;
			else if(pUser->trafficControl[flowId].scaleFactor > 150)
				pUser->trafficControl[flowId].scaleFactor = 150;
#endif

			pUser->trafficControl[flowId].isTrafficCtl = false;
			pUser->trafficControl[flowId].isPktTxStop = false;
            if(g_isTrafficCtl == 0)
                continue;

			lossPktCnt = pUser->trafficControl[flowId].pktLossCnt;
			sendPktCnt = DISTANCESN(pUser->trafficControl[flowId].ssnBegin, pUser->trafficControl[flowId].ssnEnd);
			MPDLOG(pUser->uid, LLV_DEBUG, "MPD_TrafficControl flowId %d:lossPktCnt %d, ssnBegin %d, ssdEnd %d,sendPktCnt %d", 
				flowId, lossPktCnt, pUser->trafficControl[flowId].ssnBegin, pUser->trafficControl[flowId].ssnEnd, sendPktCnt);
			pUser->trafficControl[flowId].ssnBegin = pUser->trafficControl[flowId].ssnEnd;

            //发包增长系数
			if(pUser->trafficControl[flowId].sackNum)
			{
			    if(lossPktCnt*100 > sendPktCnt*19)
                {         
				    pUser->trafficControl[flowId].scaleFactor = pUser->trafficControl[flowId].scaleFactor*60/100;  //错包太多了
                }
                else
                {
                    pUser->trafficControl[flowId].scaleFactor = pUser->trafficControl[flowId].scaleFactor*95/100;
                }
                
			}
			else
			{
			    U32 tmp = pUser->trafficControl[flowId].scaleFactor*105/100;
                if(tmp > (pUser->trafficControl[flowId].scaleFactor + 5))
				    pUser->trafficControl[flowId].scaleFactor = tmp;
                else
                    pUser->trafficControl[flowId].scaleFactor += 5;
			}

			//pUser->trafficControl[flowId].scaleFactor出现到0的情况
			if(pUser->trafficControl[flowId].scaleFactor < 90)
				pUser->trafficControl[flowId].scaleFactor = 90;
			else if(pUser->trafficControl[flowId].scaleFactor > 110)
				pUser->trafficControl[flowId].scaleFactor = 110;


			//速率加速度
			MPDLOG(pUser->uid, LLV_DEBUG, "MPD_TrafficControl flowId %d: pktRatePre %d, pktRate %d, lossThrsh %d, accNum %d, txWinEmptyCnt %d, scaleFactor %d", 
				flowId, pUser->trafficControl[flowId].pktRatePre, pUser->trafficControl[flowId].pktRate, pUser->trafficControl[flowId].lossThrsh,
				pUser->trafficControl[flowId].acceleratedCnt, pUser->schdPara[flowId].txWinEmptyCnt, pUser->trafficControl[flowId].scaleFactor);

			if(pUser->trafficControl[flowId].quitTrafficCtlCnt == 0) //不保持时候才需要计算加速度
			{
				U32 pktIncre = (pUser->trafficControl[flowId].pktRatePre*105 + 99)/100;  //速率加速度默认5%
				U32 pktIncre1 = (pUser->trafficControl[flowId].pktRatePre*95 + 99)/100;  //速率加速度默认5%			
				pUser->trafficControl[flowId].pktRatePre = pUser->trafficControl[flowId].pktRate;
	            if((pUser->trafficControl[flowId].pktRate < pktIncre) && (pUser->trafficControl[flowId].pktRate > pktIncre1))
	            {
	                pUser->trafficControl[flowId].acceleratedCnt++;
	            }
	            else
	            {
	                pUser->trafficControl[flowId].acceleratedCnt = 0;
	            }
			}

            //计算滑动平均速率
            
            rateAvg = (pUser->trafficControl[flowId].pktRateAvg * (g_schdCfg.scalePktV-2) + 2*pUser->trafficControl[flowId].pktRate)/g_schdCfg.scalePktV;
            pUser->trafficControl[flowId].pktRateAvg = rateAvg;
            if(rateAvg < pUser->trafficControl[flowId].pktRate)
            {   
                rateAvg = pUser->trafficControl[flowId].pktRate;
            }
            
			if(((lossPktCnt*100 > sendPktCnt*pUser->trafficControl[flowId].lossThrsh) && (lossPktCnt > 3)) || 
                ((pUser->trafficControl[flowId].acceleratedCnt > 2) && (pUser->schdPara[flowId].txWinEmptyCnt == 0)))
				//(pUser->trafficControl[flowId].pktRate < pktIncre))  //错包率超门限，或加速度下降
			{
			    //for test
			    if(sendPktCnt < 60)
                    MPDLOG(pUser->uid, LLV_INFO, "MPD_TrafficControl isTrafficCtl error flowId %d: but sendPktCnt %d", 
						flowId, sendPktCnt);
				pUser->trafficCtlTime = g_time;
                pUser->trafficControl[flowId].acceleratedCnt = 0;
				pUser->trafficControl[flowId].isTrafficCtl = true;
                pUser->trafficControl[flowId].quitTrafficCtlCnt = 1;
				//pUser->trafficControl[flowId].pktRate = pUser->trafficControl[flowId].pktCntTx;				
				pUser->trafficControl[flowId].throuputLimit = rateAvg*pUser->trafficControl[flowId].scaleFactor/100;
					//pUser->trafficControl[flowId].pktRate*pUser->trafficControl[flowId].scaleFactor/100;
                if(pUser->trafficControl[flowId].throuputLimit < 10)
                    pUser->trafficControl[flowId].throuputLimit = 10;
				//pUser->trafficControl[flowId].isPktTxStop = false;
				MPDLOG(pUser->uid, LLV_INFO, "MPD_TrafficControl isTrafficCtl flowId %d: throuputLimit %d", 
						flowId, pUser->trafficControl[flowId].throuputLimit);
			}
            else if(pUser->trafficControl[flowId].quitTrafficCtlCnt > 0)  
            {
                MPDLOG(pUser->uid, LLV_INFO, "MPD_TrafficControl isTrafficCtl flowId %d: throuputLimit %d, quitTrafficCtlCnt %d", 
						flowId, pUser->trafficControl[flowId].throuputLimit, pUser->trafficControl[flowId].quitTrafficCtlCnt);
                if(pUser->trafficControl[flowId].quitTrafficCtlCnt < 10)
                {
                    U32 tmpThrouput;
                    pUser->trafficCtlTime = g_time;
                    pUser->trafficControl[flowId].isTrafficCtl = true;      
                       
                    //tmpThrouput = pUser->trafficControl[flowId].pktRate*pUser->trafficControl[flowId].scaleFactor/100;
                    tmpThrouput = rateAvg*pUser->trafficControl[flowId].scaleFactor/100;
                    if(pUser->trafficControl[flowId].throuputLimit < tmpThrouput)
                        pUser->trafficControl[flowId].throuputLimit = tmpThrouput;
                    
                    pUser->trafficControl[flowId].quitTrafficCtlCnt++;

                }
                else
                {
                    pUser->isAllFlowTrafficCtl = false;
                    pUser->trafficControl[flowId].quitTrafficCtlCnt = 0;
                }
				//pUser->trafficControl[flowId].acceleratedCnt = 0;  //保持时候不判断加速度
            }
			else
			{
				pUser->isAllFlowTrafficCtl = false;
			}

			pUser->throuputTotal += pUser->trafficControl[flowId].throuputLimit;  //pktCntTx;
			pUser->trafficControl[flowId].pktLossCnt = 0;
			pUser->trafficControl[flowId].pktCntTx = 0;  //下一个周期的调度包个数
			pUser->trafficControl[flowId].pktRate = 0;
			pUser->trafficControl[flowId].sackNum = 0;
    	}
	}
}
#endif

U8 MPD_SCHD_ChooseStream(stUserInfo* pUser)
{
    U8  flowId = 0;
    U8  result = INVALID_STREAM;
    U32 wCntFlow;
    U32 minWCnt = 0xFFFFFFFF;  //large enough
    U32 histNum = 0;
	U32 tmp;

#ifdef TRAFFIC_CTL
	if(pUser->isMpStopSched)  //走到这里就会调度，不return
	{
		MPDLOG(pUser->uid, LLV_ERROR, "MPD_SCHD_ChooseStream but isMpStopSched, isAllFlowTrafficCtl %d", pUser->isAllFlowTrafficCtl);
	}
	pUser->isMpStopSched = true;	
#endif

#ifdef SCHD_BALANCE
    for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
    {
        if( (!pUser->schdPara[flowId].isValid) 
#if 0			
			|| (pUser->schdPara[flowId].isSchedValid == false)
#endif
		)
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
            continue;
        }
#ifdef TRAFFIC_CTL
		if(pUser->trafficControl[flowId].isTrafficCtl)
		{
			if(pUser->trafficControl[flowId].isPktTxStop)
			{
			    MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: trafficCtl isPktTxStop throuputLimit %d", flowId, pUser->trafficControl[flowId].throuputLimit);
				continue;
			}
			else if(pUser->trafficControl[flowId].pktCntTx >= pUser->trafficControl[flowId].throuputLimit)
			{
				pUser->trafficControl[flowId].isPktTxStop = true;
                MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: isPktTxStop pktCntTx %d", flowId, pUser->trafficControl[flowId].pktCntTx);
			}
			else
			{
				pUser->isMpStopSched = false;
			}
		}
		else
		{
			pUser->isMpStopSched = false;
		}		
#endif

#ifdef SCHD_DELAY
		histNum = MPD_getHistAcnt(pUser,flowId);
		wCntFlow = pUser->schdPara[flowId].InhertDelay;  //ms
		if((flowId == 0) && (g_isMdelayMin))
		{
			if(pUser->schdPara[flowId].InhertDelay > g_InhertDelay)
				wCntFlow = g_InhertDelay;
		}

		if(pUser->schdPara[flowId].PktAvg > 0)
		{
			wCntFlow += ((histNum*1000)/pUser->schdPara[flowId].PktAvg);

			if((flowId == 0) && (g_MminusPkt != 0))
			{	
				tmp = (g_MminusPkt*1000)/pUser->schdPara[flowId].PktAvg;
				if(wCntFlow > tmp)
					wCntFlow -= tmp;
				else
					wCntFlow = 0;
			}
		}
		else
		{
			MPDLOG(pUser->uid, LLV_INFO, "MPD_SCHD_ChooseStream flowId %d:warning PktAvg is 0, wCntFlow %d, histA %d, InhertDelay %d",
				flowId ,wCntFlow, histNum, pUser->schdPara[flowId].InhertDelay);
		}
		
		MPDLOG(pUser->uid, LLV_INFO, "MPD_SCHD_ChooseStream flowId %d: wCntFlow %d, histA %d, PktAvg %d, InhertDelay %d",
				flowId ,wCntFlow, histNum, pUser->schdPara[flowId].PktAvg, pUser->schdPara[flowId].InhertDelay);
#else

#ifdef SCHD_BALANCE_NEW
#if 0 
        wCntFlow = (MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId)+1) * pUser->schdPara[flowId].ModifyFactor;
#else
        wCntFlow = (MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId)) * pUser->schdPara[flowId].ModifyFactor;
        if(flowId == 0)  //mcWill, cwy add
        {
            wCntFlow = 0;
            #if 1
            if((MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId)) > g_mpdMcDelay)
                wCntFlow = (MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId) - g_mpdMcDelay) * pUser->schdPara[flowId].ModifyFactor;
            #else
            if((MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId)) > 6)
                wCntFlow = (MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId) - 6) * pUser->schdPara[flowId].ModifyFactor;
            #endif
        }
#endif

#else   
        wCntFlow = MPD_getTxWinUnackCnt(pUser, flowId) + MPD_getTxWinRetransCnt(pUser, flowId);
#endif

		MPDLOG(pUser->uid, LLV_INFO, "MPD_SCHD_ChooseStream flowId %d: wCntFlow %d, ModifyFactor %d, uackCnt %d, txRetransCnt %d, g_mpdMcDelay %d",
		flowId ,wCntFlow,  pUser->schdPara[flowId].ModifyFactor,MPD_getTxWinUnackCnt(pUser, flowId), MPD_getTxWinRetransCnt(pUser, flowId),g_mpdMcDelay);
#endif



        if (wCntFlow < minWCnt)
        {
            minWCnt = wCntFlow;
            result = flowId;
        }
    }

	
#ifdef TRAFFIC_CTL
	if(result == INVALID_STREAM)
	{
		MPDLOG(pUser->uid, LLV_ERROR, "result %d:MPD_SCHD_ChooseStream invalid flow", result);
	}
#endif
    pUser->schdPara[result].isSendPkt = true;
    pUser->schdPara[result].PktChooseCnt++;
#ifdef TRAFFIC_CTL
	pUser->trafficControl[result].pktCntTx++;
#endif
#else
    for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
    {
        if (!pUser->schdPara[flowId].isValid)
        {
            MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
            continue;
        }
#ifdef SCHD_NEW
        wCntFlow = pUser->schdPara[flowId].wCnt + pUser->schdPara[flowId].ModifyFactor;
#else
        wCntFlow = pUser->schdPara[flowId].wCnt + pUser->schdPara[flowId].Nweight;
#endif
        if (wCntFlow < minWCnt)
        {
            minWCnt = wCntFlow;
            result = flowId;
        }
        MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: wCntFlow %5d minWCnt %10d result %d||wCnt %5d Nweight %5d RTTFlow %5d RTTFlowMax %5d", 
            flowId, 
            wCntFlow,
            minWCnt,
            result,
            pUser->schdPara[flowId].wCnt,
#ifdef SCHD_NEW
            pUser->schdPara[flowId].ModifyFactor,
#else
            pUser->schdPara[flowId].Nweight,
#endif
            pUser->schdPara[flowId].RTTFlow,
            pUser->RTTFlowMax);
    }

    if (result == INVALID_STREAM)
        return result;
#ifdef SCHD_NEW
    pUser->schdPara[result].wCnt += pUser->schdPara[result].ModifyFactor;
#else
    pUser->schdPara[result].wCnt += pUser->schdPara[result].Nweight;
#endif
    MPDLOG(pUser->uid, LLV_DEBUG, "change result flow[%d] wCnt %d", result, pUser->schdPara[result].wCnt);
    //get least wCnt after plus Nweight
    minWCnt = 0x7FFFFFFF;
    for (flowId = 0; flowId<NUM_SUBFLOW; flowId++)
    {
        minWCnt = min(pUser->schdPara[flowId].wCnt, minWCnt);
    }
    if (minWCnt > g_schdCfg.wCntOverflowTh)
    {
        for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
        {
            pUser->schdPara[flowId].wCnt -= g_schdCfg.wCntOverflowTh;
        }
        MPDLOG(pUser->uid, LLV_DEBUG, "all flow wCnt overflow, dec all wCnt to %u %u %u.",
            pUser->schdPara[0].wCnt, pUser->schdPara[1].wCnt, pUser->schdPara[2].wCnt);
    }
#endif
    return result;
}
