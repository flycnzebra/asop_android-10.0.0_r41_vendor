/*******************************************************************************
 *              (C) Copyright 2018: Xinwei Telecom
 *
 * FILENAME: mpd_sack.h
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
#ifndef __MPD_SACK_H__
#define __MPD_SACK_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "mpd.h"
/******************************************************************************/
#define MAX_SACK_NUM   11
/******************************************************************************/

/******************************************************************************/
void MPD_SACKInit(stUserInfo* pUser);
//void MPD_processACK(stUserInfo* pUser, U8 sfId, U16 ackSSN);
//void MPD_parserSACK(stUserInfo* pUser, U8 sfId, U8* pSack);
#ifdef __cplusplus
};
#endif
#endif // __MPD_SACK_H__
