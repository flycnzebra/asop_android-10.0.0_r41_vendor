/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_schd.h
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
#ifndef __MPD_SCHD_H__
#define __MPD_SCHD_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "mpd.h"
#include "mpd_tx.h"
/******************************************************************************/
#define INVALID_STREAM       0xFF
#define TABLE_SIZE           128*NUM_SUBFLOW+1
/******************************************************************************/
typedef struct
{
    U8  scaleRTTFlow;   //smoothing factor = 1/2^alphaScale
#ifdef SCHD_NEW
    U8  scaleRTTDev;
    U8  scaleRTO0;
    U8  scaleRTO1;
    U8  scaleModifyFacktor;
    U16 linkThrptStatisPeriod;
    U16 updateAcklatencyPeriod;
    U16 updateModifyFactorPeriod;
#endif
#if (defined SCHD_NEW || defined SCHD_BALANCE_NEW)
    U8  scalePktV;
#endif
    U16 weightUpdatePeriod;
    U32 NN;
    S32 wCntOverflowTh;

}stSchdCfg;

/******************************************************************************/
void MPD_SCHD_Init(stUserInfo* pUser);
void MPD_SCHD_CfgInit();
void MPD_SCHD_setAlgVar(U8 alpha, U16 period, U32 step, U32 cntTh);
void MPD_SCHD_setFlowValid(stUserInfo* pUser, U32 flowId, U32 flag);
U8   MPD_SCHD_getFlowValid(stUserInfo* pUser, U8 flowId);
void MPD_SCHD_WeightUpdate(stUserInfo* pUser);
U8   MPD_SCHD_ChooseStream(stUserInfo* pUser);
void MPD_CHECK_Txwin_Empty(stUserInfo* pUser);
#ifdef TRAFFIC_CTL
void MPD_TrafficCtlInit(stUserInfo* pUser);
void MPD_TrafficControl(stUserInfo* pUser);
#endif
#ifdef __cplusplus
};
#endif
#endif // __MPD_SCHD_H__
