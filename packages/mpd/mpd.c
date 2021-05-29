/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd.c
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
#include "mpd_external.h"
#include "mpd_schd.h"
#include "mpd_sack.h"
#include <stdio.h>
#include <stdarg.h>
#ifdef MAG
#include "mag_pub.h"
#endif

#ifndef MAG
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#endif
/******************************************************************************/
#define MAX_LOG_LENGTH    300
/******************************************************************************/

/******************************************************************************/
U32     g_time;
U32     g_freeNum = 0;
#ifndef MAG
U32     utAddressID;
RAT_Env g_mpcFun;
stMPDFunc g_mpdFun;
sem_t   lock_mpd;
#endif
U32     g_printLvl = LLV_DEBUG;
char    g_Version[10] = "0.0.2.9";
S32     g_dev2SfId[5] = {-1,0,1,-1,2};  //devId 1 -- sfId 0; devId 2 -- sfId 1; devId 4 -- sfId 2;  -1:invalid
S32     g_sfId2Dev[NUM_SUBFLOW] = {1,2,4};
stUserInfo* g_pUserInfo[MAX_USER_NUM];
U32     g_totalUserNum = 0;
extern stSchdCfg  g_schdCfg;
/******************************************************************************/
extern void MPD_RxWinInit(stUserInfo* pUser);
extern void MPD_DataWinTimeOutCheck(stUserInfo* pUser);
extern void MPD_SackPeriodSend(stUserInfo* pUser);
extern S32 MPD_DoParseMPData(stUserInfo* pUser, const char * buffer, U32 datalen);
extern S32 MPD_DoSendNewTransData(stUserInfo* pUser, U8 *dataAddr, S32 length);
extern void MPD_ClearSubWin(stUserInfo* pUser, S32 index);
extern void MPD_ClearDataWin(stUserInfo* pUser);
extern void MPD_ChangeDataWinFlowId(stUserInfo* pUser, U8 flowId);
extern void MPD_CheckAck(stUserInfo* pUser);
#ifdef SCHD_DELAY
extern void MPD_SCHD_DelayUpdate(stUserInfo* pUser);
#endif
#ifdef MAG
extern int  OnPostPreMPDataToTransport (unsigned int addressID, const char * buffer, size_t datalen, size_t offsize);
extern int  OnSendMPDataComplete(unsigned int addressID, int fd, const char * buffer, size_t datalen, int release);
extern void do_plog(unsigned int addressID, int priority, char *format, ...);
extern int  OnNotifyException(unsigned int addressID, int netType, int exceptionCode);
//����һ���ڴ棬������m_buf�ĵ�ַ��m_bufĿǰ�ǹ̶���С����Ҫ����size�����2048
extern void* MAG_MsgMemCreat(size_t size);
//��ӦMAG_MsgMemCreat������m_buf�ĵ�ַ���ͷ��ڴ�
extern void MAG_MsgMemfree(void *memblock);
//����m_buf�ĵ�ַ������DataPtr�ĵ�ַ���õ�ַǰԤ��256���ֽ�
extern void* MAG_GetMsgDataPtr(void *memblock);
//���ݴ�С������һ���ڴ棬�����ڴ��ַ��rte_malloc
extern void* MAG_Malloc(size_t size);
//��ӦMAG_Free,�����ڴ��ͷ�
extern void* MAG_Free(void* memblock);
extern unsigned long GetRTTC(unsigned int addressID,int netType);  //mpd���ϲ��ȡָ������RTT
#endif
#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
void MPD_SCHD_PktAvgUpdate(stUserInfo* pUser);
#endif
extern void MPD_SCHD_RttUpdate(stUserInfo* pUser);


/******************************************************************************/
void MPD_UserInfoInit(U32 addressId, U32 uid)
{
    stUserInfo* pUser = g_pUserInfo[addressId];
    MPD_SCHD_Init(pUser);
    MPD_TxWinInit(pUser);
    MPD_SACKInit(pUser);
    MPD_RxWinInit(pUser);
    pUser->addressID = addressId;
    pUser->uid = uid;
    pUser->SendSackTime = g_time + SackTimeOut;
    pUser->lastPackageTime = g_time + MPD_IDLE_TH;
    pUser->totalToAckNum = 0;
    pUser->rxDsnErr = 0;
    pUser->exceptionCode[0] = 0; //wangweiTest
    pUser->exceptionCode[1] = 0;
    pUser->exceptionCode[2] = 0;
#ifdef TRAFFIC_CTL
	MPD_TrafficCtlInit(pUser);
	pUser->txPktCnt = 0;
	pUser->txPktFreeCnt = 0;
#endif
    pUser->activeFlowNum = 0;
}

U32 MPD_CreatUserSession(U32 uid) 
{
    U32 idx = 0;
    for(; idx<MAX_USER_NUM; idx++)
    {
        if(g_pUserInfo[idx] != NULL)
            continue;
        g_pUserInfo[idx] = (stUserInfo*)MPD_memAlloc(sizeof(stUserInfo));
        if(g_pUserInfo[idx] == NULL)
        {
            MPDLOG(0xffffffff, LLV_ERROR,"MPD_CreatUserSession alloc NULL,idx %d, g_totalUserNum %d", idx, g_totalUserNum);
            return 0;
        }
        g_totalUserNum++;
        MPD_UserInfoInit(idx, uid);
		MPDLOG(uid, LLV_NOTIFY,"MPD_CreatUserSession idx %d, g_totalUserNum %d, uid %d", idx, g_totalUserNum, uid);
        return idx;
    }
    MPDLOG(uid, LLV_ERROR, "error: MPD_CreatUserSession, alloc userInfo failed");
    return (U32)-1;
}

void MPD_DeleteUserSession(unsigned int addressID)
{
    U8 i;
    stUserInfo* pUser = g_pUserInfo[addressID];
    if(addressID > MAX_USER_NUM || g_totalUserNum == 0)
    {
        MPDLOG(0xffffffff, LLV_ERROR, "error: MPD_DeleteUserSession %d overflow,totalUserNum %d", addressID, g_totalUserNum);
        return;
    }
    if(pUser == NULL)
    {
        MPDLOG(0xffffffff, LLV_ERROR, "error: MPD_DeleteUserSession %d %d NULL", addressID,g_totalUserNum);
        return;
    }
    MPDLOG(pUser->uid, LLV_NOTIFY, "MPD_DeleteUserSession() addressID %d", addressID);
    MPD_ClearDataWin(pUser);
    for(i=0;i<NUM_SUBFLOW;i++)
    {
        if(MPD_SCHD_getFlowValid(pUser, i))
        {
            MPD_clearTxWin(pUser, i);
        }
    }
    MPD_memFree(pUser);
    g_totalUserNum--;
    g_pUserInfo[addressID] = NULL;
}

S32 MPD_GetSubFlowId(U32 deviceId)
{
	S32 ret = -1;
	if (deviceId>4)
	{
		MPDLOG(0xffff, LLV_ERROR, "GetSubFlowId error: deviceId %d", deviceId);
		return -1;
	}
	ret = g_dev2SfId[deviceId];
	if (ret == -1)
	{
		MPDLOG(0xffff, LLV_ERROR, "GetSubFlowId error: deviceId %d", deviceId);
	}
	return ret;
}

void MPD_DoSubFlowStateInd(stUserInfo* pUser, U32 deviceId, U32 state)
{
    U8  flag = 0;
    U8  i;
	S32 flowId = MPD_GetSubFlowId(deviceId);
    U8  flowState = 0;
    bool isChange = false;
    U8  flagOld = 0;

#if 1
	if(deviceId == 0)   //�򿪻�رն���
	{
		MPDLOG(pUser->uid, LLV_NOTIFY,"MPD_DoSubFlowStateInd setMpStateInd state %d", state);
		for (i=0; i<NUM_SUBFLOW; i++)
		{
			if(MPD_SCHD_getFlowValid(pUser, i))
			{
				//error
				MPDLOG(pUser->uid, LLV_NOTIFY,"setMpStateInd but flowId %d is valid", i);
				MPD_clearTxWin(pUser, i);
				MPD_ClearSubWin(pUser, i);
			}
		}
		MPDLOG(pUser->uid, LLV_WARNING,"Clear DataWin and set txDataSn 0");
		MPD_ClearDataWin(pUser);
		pUser->txDataSn = 0;
        pUser->rxDsnErr = 0;
#ifdef TRAFFIC_CTL
		MPD_clearWaitQueue(pUser);	
		MPD_TrafficCtlInit(pUser);	
#endif		

        pUser->activeFlowNum = 0;
		return;
	}	
#endif

	MPDLOG(pUser->uid, LLV_NOTIFY,"setSbuFLowStateInd flowId %d, deviceId %d, NEWstate %d, CURstate %d",
	            flowId, deviceId, state, pUser->schdPara[flowId].isValid);
	if (flowId == -1)
		return;
    
	
#if 0
	if(state == MP_NOSEND)
	{
		flowState = 1;
		pUser->schdPara[flowId].isSchedValid = false;
	}

	
	if(state == MP_ENABLE)
	{
		//flowState = 1;		
		pUser->schdPara[flowId].isSchedValid = true;
		
		if(MPD_SCHD_getFlowValid(pUser, flowId) && (pUser->schdPara[flowId].isSchedValid == false))
		{
			//pUser->schdPara[flowId].isSchedValid = true;
			return;
		}
		else
		{
			//error
			MPDLOG(pUser->uid, LLV_ERROR, "MPD_DoSubFlowStateInd but flowId %d flowValid %d isSchedValid %d", 
			flowId, MPD_SCHD_getFlowValid(pUser, flowId), pUser->schdPara[flowId].isSchedValid);
			//pUser->schdPara[flowId].isSchedValid = true;
			//flowState = 1;
		}
	}
#endif

    if(state == MP_ENABLE)
    {
        flowState = 1;
    }

    if(MPD_SCHD_getFlowValid(pUser, flowId) != flowState)
    {
        isChange = true;
        if(flowState)
        {
            for (i=0; i<NUM_SUBFLOW; i++)
            {
                pUser->schdPara[i].wCnt = g_schdCfg.NN;
                pUser->schdPara[i].RTTFlow = 1;
            }
        }
    }
    for(i=0;i<NUM_SUBFLOW;i++)
    {
        flagOld |= MPD_SCHD_getFlowValid(pUser, i);
    }
    MPD_SCHD_setFlowValid(pUser, flowId,flowState);
	if (flowState == 0) //shut down subflow, clear data in subflow
	{
		MPD_clearTxWin(pUser, flowId);
        MPD_ClearSubWin(pUser, flowId);
        MPD_ChangeDataWinFlowId(pUser, flowId);
        for(i=0;i<NUM_SUBFLOW;i++)
        {
            flag |= MPD_SCHD_getFlowValid(pUser, i);
        }
        if(isChange)
        {
            if(flag == 0)
            {
                MPDLOG(pUser->uid, LLV_WARNING,"Clear DataWin and set txDataSn   0");
                MPD_ClearDataWin(pUser);
                pUser->txDataSn = 0;
                pUser->rxDsnErr = 0;
            }
            if(flagOld == 0)
            {
                pUser->lastPackageTime = g_time + MPD_IDLE_TH;
                pUser->totalToAckNum = 0;
            }
        }
    }

    pUser->activeFlowNum = 0;
    for(i=0;i<NUM_SUBFLOW;i++)
    {
        if(MPD_SCHD_getFlowValid(pUser, i))
        {
            pUser->activeFlowNum++;
        }
    }
}

S32 MPD_GetDeviceId(U32 flowId)
{
    if (flowId>=NUM_SUBFLOW)
    {
        //MPDLOG(LLV_ERROR, "MPD_GetDeviceId error: sfId %d", flowId);
		return -1;
    }
	return g_sfId2Dev[flowId];
}
void MPD_periodCheck()
{
    U32 idx;
    U32 userNum = 0;
    //U8* pTemp = NULL;
    
    g_time++;
#ifndef MAG	
    if(g_time%200 == 0)
    {
     //pTemp = (U8*)MPD_MsgMemCreat(256);
     //(*(g_mpcFun.Free_ResendBuf))(pTemp, 99);

      //pTemp = (U8*)MPD_MsgMemCreat(256);
     //g_mpcFun.Free_ResendBuf(pTemp, 99);
     MYLOGP(0xffff, LLV_WARNING,"MPD_periodCheck g_time %d retransFreeNum %d, ver %s", g_time, g_freeNum, MPD_GetVersion());
    //MPDLOG(0,LLV_WARNING, "MPD_periodCheck  ");
    }
    
#endif
    //MPDLOG(0,LLV_WARNING, "MPD_periodCheck g_time %d v1.0", g_time);


    //for user
    for(idx=0; idx<MAX_USER_NUM; idx++)
    {
        if(g_pUserInfo[idx] == NULL)
            continue;
        if(userNum >= g_totalUserNum)
            break;
#ifdef MAG
        MPD_CheckTxWinPdata(g_pUserInfo[idx]);
#endif
#if 1
		MPD_CHECK_Txwin_Empty(g_pUserInfo[idx]);
#endif

#ifdef TRAFFIC_CTL
		MPD_TrafficControl(g_pUserInfo[idx]); //���ڼ��������Ƿ��������״̬		
#endif        
#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
        MPD_SCHD_PktAvgUpdate(g_pUserInfo[idx]);
#endif
#ifdef SCHD_DELAY
		MPD_SCHD_DelayUpdate(g_pUserInfo[idx]);
#endif
#ifndef SCHD_BALANCE        
        MPD_SCHD_WeightUpdate(g_pUserInfo[idx]);
#endif
#ifdef TRAFFIC_CTL
        MPD_DoSendWaitQueueData(g_pUserInfo[idx]); //���ȴ������еĴ��������ݵ��ȷ���
#endif
        MPD_timeoutTxWinSuspend(g_pUserInfo[idx]);
#ifndef NO_RANK        
        MPD_DataWinTimeOutCheck(g_pUserInfo[idx]);
#endif
        MPD_SackPeriodSend(g_pUserInfo[idx]);
        userNum++;
#ifdef CHECK_ACK
        MPD_CheckAck(g_pUserInfo[idx]);
#endif
        MPD_TxWinRetransCheck(g_pUserInfo[idx]);
#ifdef MAG
        MPD_CheckTxWinPdata(g_pUserInfo[idx]);
#endif
#ifndef MAG
        MPD_CheckNoTraffic(g_pUserInfo[idx]);
#endif
#if 1  //ndef MAG
        MPD_SCHD_RttUpdate(g_pUserInfo[idx]);
#endif
#if 0
		MPD_CHECK_Txwin_Empty(g_pUserInfo[idx]);
#endif
    }
}

char* MPD_GetVersion()
{
    return g_Version;
}

void MPD_setPrintLvl(U8 logLevel)
{
    g_printLvl = logLevel;
}

void MPD_log(U32 uid, U8 log_level, const char* format, ...)
{
    char buffer[MAX_LOG_LENGTH] = {0};
    va_list args;
    U32 len;
#ifndef MAG
    if(log_level >= LLV_DEBUG)
    {
        return;
    }
#endif	
    va_start(args, format);
    len = vsprintf(buffer, format, args);
    if (len >= MAX_LOG_LENGTH)
    {
        sprintf(buffer, "buffer overflow!!!!,need expand buffer");
        //buffer[MAX_LOG_LENGTH-1] = '\0';
    }
#ifdef WIN32
    if (log_level <= g_printLvl)
    {
        FILE* pf = fopen("..\\..\\test.log", "a+");
        if (pf)
        {
            fprintf(pf,"%s", buffer);
        }
        fclose(pf);
        //printf("%s", buffer);
    }
#else
#ifdef MAG
        //do_plog(uid, log_level, "%s", buffer);
        do_plog(uid, log_level, buffer);
#else
        //add the time for UT
        
        //time_t rawtime; 
        //struct tm * timeinfo; 
        //struct timeval tv;
        //struct timeb stTimeb;

        //ftime(&stTimeb);
        //time(&rawtime ); 
        //gettimeofday(&tv, NULL);
        //timeinfo = localtime(&tv.tv_sec); 

		//char buffer1[MAX_LOG_LENGTH+60] = {0};
		//sprintf(buffer1,"time %02d-%02d %02d:%02d:%02d.%03d, %s", 
            //timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,tv.tv_usec, buffer);
		(*g_mpcFun.do_plog)(log_level, "%s", buffer);
        //printf("time %02d-%02d %02d:%02d:%02d.%03d, %s\n", 
            //timeinfo->tm_mon+1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,tv.tv_usec, buffer);
        //(*g_mpcFun.do_plog)(log_level, "%s", buffer);
#endif
#endif
}

#ifdef MAG
S32 MPD_ParseMPData (U32 addressID, const char* buffer,U32 datalen)
{
    stUserInfo* pUser = g_pUserInfo[addressID];
    if(pUser != NULL)
    {
        return MPD_DoParseMPData (pUser, buffer, datalen);
    }
    else
    {
        MPDLOG(addressID, LLV_ERROR, "error: addressID %d invalid!", addressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", buffer);
        MPD_MsgMemfree(buffer);
        return -1;
    }
}
S32 MPD_SendNewTransData(U32 addressID, U8 *dataAddr, S32 length)
{
    stUserInfo* pUser = g_pUserInfo[addressID];
    if(pUser != NULL)
    {
        return MPD_DoSendNewTransData(pUser, dataAddr, length);
    }
    else
    {
        MPDLOG(addressID, LLV_ERROR, "error: addressID %d invalid!", addressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", dataAddr);
        MPD_MsgMemfree(dataAddr);
    }
}

void MPD_SubFlowStateInd(U32 addressID, U32 deviceId, U32 state)
{
    stUserInfo* pUser = g_pUserInfo[addressID];
    if(pUser != NULL)
    {
        MPD_DoSubFlowStateInd(pUser, deviceId, state);
    }
    else
    {
        MPDLOG(addressID, LLV_ERROR, "error: addressID %d invalid!", addressID);
    }
    
}

void MPD_Init()
{
    g_time = 0;
    MPD_SCHD_CfgInit();
    memset(g_pUserInfo, 0, sizeof(stUserInfo*)*MAX_USER_NUM);
}

#else // UT

U32 MPD_ParseMPData(const char* buffer,U32 datalen)
{
    //MYLOG(0xffff, LLV_WARNING,"MPD_ParseMPData() is in");

    stUserInfo* pUser = g_pUserInfo[utAddressID];
    if(pUser != NULL)
    {
        //MYLOG(0xffff, LLV_WARNING,"MPD_ParseMPData() is out");
        return MPD_DoParseMPData (pUser, buffer, datalen);
    }
    else
    {
        MPDLOG(utAddressID, LLV_ERROR, "error: addressID %d invalid!", utAddressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_ParseMPData,error: addressID %d invalid!", utAddressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", buffer);
        MPD_MsgMemfree(buffer);
        return -1;
    }
}

S32 MPD_SendNewTransData(U8 *dataAddr, S32 length)
{
    //MYLOG(0xffff, LLV_WARNING, "MPD_SendNewTransData() is called");

    stUserInfo* pUser = g_pUserInfo[utAddressID];
    if(pUser != NULL)
    {
        MPDLOG(utAddressID, LLV_WARNING, "MPD_SendNewTransData: addressID %d, len %d!", utAddressID, length);
        return MPD_DoSendNewTransData(pUser, dataAddr, length);
    }
    else
    {
        MPDLOG(utAddressID, LLV_ERROR, "error: addressID %d invalid!", utAddressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_SendNewTransData, error: addressID %d invalid!", utAddressID);
        MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", dataAddr);
        MPD_MsgMemfree(dataAddr);
    }
    return 0;
}

void MPD_SubFlowStateInd(U32 deviceId, U32 state)
{
    stUserInfo* pUser = g_pUserInfo[utAddressID];
    if(pUser != NULL)
    {
        //sem_wait(&lock_mpd);
        MPD_DoSubFlowStateInd(pUser, deviceId, state);
        //sem_post(&lock_mpd);
    }
    else
    {
        MPDLOG(utAddressID, LLV_ERROR, "error: addressID %d invalid!", utAddressID);
    }
    
}

unsigned long MPD_GetRTT(int netType)
{
    stUserInfo* pUser = g_pUserInfo[utAddressID];
    U32  rtt = 5;
    S32 flowId = MPD_GetSubFlowId(netType);
    
    if(pUser != NULL)
    {
        rtt = pUser->schdPara[flowId].InhertDelay;   
        //MYLOGP(0xffff, LLV_WARNING, "MPD_GetRTT: netType %d, flowId %d, rtt %d;", netType, flowId, rtt);
    }
    else
    {
        MPDLOG(utAddressID, LLV_ERROR, "error: addressID %d invalid!", utAddressID);
    }    

    return rtt;
}

stMPDFunc* MPD_Init(U32 uid, RAT_Env* funPtr)
{
    g_time = 0;
    U8* pTemp = NULL;
    memset(g_pUserInfo, 0, sizeof(stUserInfo*)*MAX_USER_NUM);
    //MPDLOG(0xffff, LLV_WARNING,"MPD_Init completed 1");
    MPD_SCHD_CfgInit();
    utAddressID = MPD_CreatUserSession(uid);
     //MPDLOG(0xffff, LLV_WARNING,"MPD_Init completed 2");
    memcpy(&g_mpcFun, funPtr, sizeof(g_mpcFun));
     //g_mpcFun.Free_ResendBuf = funPtr->Free_ResendBuf;
     //MPDLOG(0xffff, LLV_WARNING,"MPD_Init completed 3");

    //sem_init(&lock_mpd,0,1);
    //JUST FOR TEST
     //pTemp = (U8*)MPD_MsgMemCreat(256);
     //(*(g_mpcFun.Free_ResendBuf))(pTemp, 0);
     
	g_mpdFun.version              = g_Version;
    g_mpdFun.MPD_SubFlowStateInd  = &MPD_SubFlowStateInd;
	g_mpdFun.MPD_GetVersion       = &MPD_GetVersion;
    g_mpdFun.ParseMPData          = &MPD_ParseMPData;
    g_mpdFun.MPD_SendNewTransData = &MPD_SendNewTransData;
    g_mpdFun.MPD_periodCheck      = &MPD_periodCheck;
    g_mpdFun.MPD_GetRTT           = &MPD_GetRTT;
	MPDLOG(0xffff, LLV_WARNING,"MPD_Init completed");
    return &g_mpdFun;
}
#endif

int MPD_PostPreMPDataToTransport (unsigned int addressID, const char * buffer, size_t datalen, size_t offsize)
{
#ifdef MAG
    return OnPostPreMPDataToTransport (addressID, buffer, datalen, offsize);
#else
    return (*g_mpcFun.OnPostPreMPDataToTransport)(buffer, datalen, offsize);
#endif
}

int  MPD_OnSendMPDataComplete(unsigned int addressID, int fd, const char * buffer, size_t datalen, int release)
{
#ifdef MAG
    return OnSendMPDataComplete(addressID, fd, buffer, datalen, release);
#else
    return (*g_mpcFun.OnSendMPDataComplete)(fd, buffer, datalen, release);
#endif
}

int  MPD_OnNotifyException(unsigned int addressID, int netType, int exceptionCode)
{
#ifdef MAG
    return OnNotifyException(addressID, netType, exceptionCode);
#else
    return g_mpcFun.OnNotifyException(g_sfId2Dev[netType], exceptionCode);
#endif
}

U8* MPD_memAlloc(size_t size)
{
#ifdef MAG
    return (U8*)MAG_Malloc(size);
#else
    return malloc(size);
#endif
}

void MPD_memFree(void *memblock)
{
#ifdef MAG
    MAG_Free(memblock);
#else
    free(memblock);
#endif
}


U8* MPD_MsgMemCreat(size_t size)
{
#ifdef MAG
    return (U8*)MAG_MsgMemCreat(size);
#else
    return malloc(size);
#endif
}

void MPD_MsgMemfree(void *memblock)
{
#ifdef MAG
    MPDLOG(0xFFFF, LLV_WARNING, "MAG_MsgMemfree(), data is 0x%x", memblock);
    MAG_MsgMemfree(memblock);
#else
    //MYLOG(0xffff, LLV_WARNING,"MPD_MsgMemfree address 0x%x ", memblock);
    free(memblock);
#endif
}

void MPD_RetransMsgMemfree(void *memblock, int flowId)
{
#ifdef MAG
    MPDLOG(0xFFFF, LLV_WARNING, "MPD_RetransMsgMemfree(), data is 0x%x", memblock);
    MAG_MsgMemfree(memblock);
#else

    (*g_mpcFun.Free_ResendBuf)(memblock, flowId);
    //MYLOG(0xffff, LLV_WARNING,"Free_ResendBuf, memblock 0x%x flowId %d !", memblock, flowId);
    g_freeNum++;
#endif
}

U8* MPD_GetMsgDataPtr(void *memblock, bool bOffset)
{
#ifdef MAG
    if(bOffset)
    {
        return (U8*)MAG_GetMsgDataPtr(memblock)-sizeof(stMPDHeader);
    }
    else
    {
        return (U8*)MAG_GetMsgDataPtr(memblock);
    }
#else
    return memblock;
#endif
}

#ifndef MAG
/*CPE mpd���ڼ����������շ������ж��Ƿ���ҵ�������ҵ�����ϱ���MPC*/
void MPD_CheckNoTraffic(stUserInfo* pUser)
{
	U8  flowId = 0;
    stTxWin* pTxWin;
    stRxSubWin* pRxWin;
    bool isNoTraffic = true;

	if((g_time % 200) == 0) //  3����һ��
	{
		for (flowId=0; flowId<NUM_SUBFLOW; flowId++)
		{
			if (!pUser->schdPara[flowId].isValid)
            {
                //MPDLOG(pUser->uid, LLV_DEBUG, "flowId %d: invalid flow", flowId);
                continue;
            }

            pTxWin = &(pUser->txWindow[flowId]);
            pRxWin = &(pUser->RxSubWin[flowId]);


            if(!isNoTraffic)  //���ǰ���ж��Ѿ�������noTraffic,������������ж��ˡ�
            {
                pTxWin->lastTxBeginSsn = pTxWin->pTxBegin->SSN;
                pRxWin->lastRxBegin = pRxWin->RxBegin;
                continue;
            }
            
            if((pTxWin->pTxBegin != pTxWin->pTxNext) || (pTxWin->pTxBegin->SSN != pTxWin->lastTxBeginSsn))
            {
                isNoTraffic = false;
                pTxWin->lastTxBeginSsn = pTxWin->pTxBegin->SSN;
                pRxWin->lastRxBegin = pRxWin->RxBegin;                
                continue;
                //return;
            }
            
            if((pRxWin->RxBegin != pRxWin->RxNext) || (pRxWin->RxBegin != pRxWin->lastRxBegin))
            {
                isNoTraffic = false;
                pRxWin->lastRxBegin = pRxWin->RxBegin;
                //return;
            }
		}

        //MYLOGP(0xffff, LLV_WARNING, "MPD_CheckNoTraffic isNoTraffic %d lastTxBeginSsn %d, %d, %d ", 
            //isNoTraffic, pUser->txWindow[0].lastTxBeginSsn, pUser->txWindow[1].lastTxBeginSsn,pUser->txWindow[2].lastTxBeginSsn);

        if(isNoTraffic)
        {
            if(pUser->RxDataWin.RxBegin != pUser->RxDataWin.RxNext)
            {
                MYLOGP(0xffff, LLV_WARNING, "MPD_CheckNoTraffic but rxDataWin isn't NULL, %d, %d ", 
                             pUser->RxDataWin.RxBegin, pUser->RxDataWin.RxNext);
                return;
            }
            else
            {
                MPD_OnNotifyException(pUser->addressID, 0, -6);  // -6��ʾ��ҵ��
            }
        }
	}

}
#endif
