

#include <stdbool.h>


int mp_data_send(char *buf,int len);

int MPC_OnSendMpDataComplete(int netType, char *buffer, int len,int release);   
int MPC_OnPostPreMpDataToTransport(const char *buffer, size_t datalen,size_t offsize);

int MPC_OnNotifyException(int netType,int exceptionCode);

void Free_ResendBuf(char *buffer,int subFlowFlag);

long getTimeStep();

int echoMpStatus(char * result_msg);
