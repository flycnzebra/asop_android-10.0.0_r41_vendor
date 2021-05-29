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
#ifdef MAG
signed int MPD_ParseMPData (unsigned int addressID, const char* buffer,unsigned int datalen);
signed int MPD_SendNewTransData(unsigned int addressID, unsigned char *dataAddr, signed int length);
void MPD_SubFlowStateInd(unsigned int addressID, unsigned int deviceId, unsigned int state);
void MPD_Init();
#else // UT
U32 MPD_ParseMPData(const char* buffer,U32 datalen);
S32 MPD_SendNewTransData(U8 *dataAddr, S32 length);
void MPD_SubFlowStateInd(U32 deviceId, U32 state);
unsigned long MPD_GetRTT(int netType);
#endif
void MPD_periodCheck();
char* MPD_GetVersion();


