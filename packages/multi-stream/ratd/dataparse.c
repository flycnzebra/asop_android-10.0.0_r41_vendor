

#include "head.h"



const char *strstri(const char*str,const char * subStr){
    int len = strlen(subStr);
    if(len=0)return NULL;

    while(*str){

        if(strncasecmp(str,subStr,len))return str;
        ++str;
    }
    
    return NULL;
}

bool decodeAppDisableReq(struct r_app_disable* r_disable,char * buffer,int len){
   
    bool result = false;

    char * strFound=strstr(buffer,NODE_SESSION_ID);
    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_disable->baseInfo.sessionId)){
       
        result = true;
    }

    return result;
}

bool decodeAppAddSubFlowReq(struct r_app_add* r_add,char * buffer,int len){

    char *strFound=strstr(buffer,NODE_NET_TYPE); 
    bool result = false;

    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_NET_TYPE,&r_add->netType)){
          
            strFound=strstr(buffer,NODE_SESSION_ID);
             if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_add->baseInfo.sessionId)){
               
                 strFound=strstr(buffer,NODE_TOKEN);
                 if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_TOKEN,&r_add->token)){
                   

                    strFound=strstr(buffer,NODE_BAND);
                    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_BAND,&r_add->band)){
                        
                         strFound=strstr(buffer,NODE_RTT);
                         if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_RTT,&r_add->rtt)){
                           
                            strFound=strstr(buffer,NODE_SUBFLOW_IP);                            
                            if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SUBFLOW_IP,&r_add->ip)){
                               
                                 strFound=strstr(buffer,NODE_NET_TYPE_NAME);
                                 if(strFound!=NULL){

                                    if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME,&r_add->netTypeName)){

                                        
                                        result = true;
                                    }else if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME_WITH_QUOTATION_MARKS,&r_add->netTypeName)){
            
                                        result = true;
                                    }else
                                        result = false;   
                                 }
                            }
                        }
                    }
                 }

             }
    }
    return result;
}


bool decodeAppDelSubFlowReq(struct r_app_del* r_del,char * buffer,int len){

    bool result = false;


             char *strFound=strstr(buffer,NODE_SESSION_ID);
             if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_del->baseInfo.sessionId)){


                strFound=strstr(buffer,NODE_NUMBER_TYPE);
                 if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_NUMBER_TYPE,&r_del->numberType)){

                     strFound=strstr(buffer,NODE_TOKEN);
                     if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_TOKEN,&r_del->token)){
                        

                        strFound=strstr(buffer,NODE_DELETE_CAUSE);
                        if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_DELETE_CAUSE,&r_del->deleteCause)){
                            result = true;                        
                        }
                     }

                 }
            }
    return result;
}
bool decodeAppTrySubFlowReq(struct r_app_try* r_try,char * buffer,int len){

    char *strFound=strstr(buffer,NODE_NET_TYPE); 
    bool result = false;

    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_NET_TYPE,&r_try->netType)){

            strFound=strstr(buffer,NODE_SESSION_ID);
             if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_try->baseInfo.sessionId)){

                 strFound=strstr(buffer,NODE_NET_TYPE_NAME);
                 if(strFound!=NULL){

                    if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME,&r_try->netTypeName)){

                        result = true;
                    }else if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME_WITH_QUOTATION_MARKS,&r_try->netTypeName)){
                        result = true;
                    }else
                        result = false;  
                 }
             }            

    }


    return result;
}

bool decodeAppMPCKeepAliveReq(struct r_app_mpc_keepalive* r_keepalive,char * buffer,int len){

    bool result = false;
    
    char * strFound=strstr(buffer,NODE_SESSION_ID);
    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_keepalive->baseInfo.sessionId)){
        result = true;
    }
    return result;
}
bool decodeAppEnablelogReq(struct r_app_enable_log* r_log,char * buffer,int len){

    bool result = false;

    char * strFound=strstr(buffer,NODE_SESSION_ID);
    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_log->baseInfo.sessionId)){

        strFound=strstr(buffer,NODE_OPERATION);        

        if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_OPERATION,&r_log->operation)){
           result = true;
        }
    }
    return result;
}
bool decodeAppEnableReq(struct r_app_enable* r_enable,char * buffer,int len){

    char *strFound=strstr(buffer,NODE_NET_TYPE); 
    bool result = false;

    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_NET_TYPE,&r_enable->netType)){

            strFound=strstr(buffer,NODE_SESSION_ID);
             if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_enable->baseInfo.sessionId)){

                 strFound=strstr(buffer,NODE_NET_TYPE_NAME);
                 if(strFound!=NULL){

                    if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME,&r_enable->netTypeName)){

                        result = true;
                    }else if(0<sscanf(strFound,RE_NODE_NET_TYPE_NAME_WITH_QUOTATION_MARKS,&r_enable->netTypeName)){
                        result = true;
                    }else
                        result = false;                 
                 }
             }            

    }

    return result;
}

bool decodeAppConfigReq(struct r_app_config* r_config,char * buffer,int len){

    char *strFound=strstr(buffer,NODE_UID); 
    bool result = false;

    if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_UID,&r_config->uid)){

        strFound=strstr(buffer,NODE_SESSION_ID);

        if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_config->baseInfo.sessionId)){

            strFound=strstr(buffer,NODE_DNS);
            if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_DNS,&r_config->dns)){
                strFound=strstr(buffer,NODE_MAG);
                if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_MAG,&r_config->mag)){

                    result = true; 
                    
                }
            }
        }    
    }
    return result;    
}

bool decodeAppReleaseMpReq(struct r_app_release_mp* r_release,char * buffer,int len){

    bool result = false;

     char *strFound=strstr(buffer,NODE_SESSION_ID);
     if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_SESSION_ID,&r_release->baseInfo.sessionId)){
        
        strFound=strstr(buffer,NODE_NUMBER_TYPE);
         if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_NUMBER_TYPE,&r_release->numberType)){
          
             strFound=strstr(buffer,NODE_TOKEN);
             if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_TOKEN,&r_release->token)){
             
                strFound=strstr(buffer,NODE_DELETE_CAUSE);
                if(strFound!=NULL&&0<sscanf(strFound,RE_NODE_DELETE_CAUSE,&r_release->deleteCause)){
                  
                  result = true;                        
                }
             }

         }
    }
    
    return result;



}


void decodeAppCommandBufferNew(void ** stCommand,char * buffer,int len){

    char *strFound;       
    int exceptioncode;
    int messageType;
    int netType;
    strFound=strstr(buffer,NODE_MESSGETYPE);
    if(strFound!=NULL&&1==sscanf(strFound,RE_NODE_MESSGETYP,&messageType)){
        switch (messageType)
        {
            case MESSAGE_TYPE_CONFIG_MP_REQ:{
                *stCommand=(struct r_app_config*)malloc(sizeof(struct r_app_config));                
                ((struct r_app_config*)*stCommand)->baseInfo.messageType = messageType;
                
                if(decodeAppConfigReq(*stCommand,buffer,len))
                    return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                
            }
                break;
            case MESSAGE_TYPE_ENABLE_MP_REQ:{
                *stCommand=(struct r_app_enable*)malloc(sizeof(struct r_app_enable));                
                ((struct r_app_enable*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppEnableReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;
            case MESSAGE_TYPE_DISABLE_MP_REQ:{

                *stCommand=(struct r_app_disable*)malloc(sizeof(struct r_app_disable));
                
                ((struct r_app_disable*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppDisableReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;
            case MESSAGE_TYPE_ADD_SUBFLOW_REQ:{

                *stCommand=(struct r_app_add*)malloc(sizeof(struct r_app_add));
                
                ((struct r_app_add*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppAddSubFlowReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;

           
            case MESSAGE_TYPE_DEL_SUBFLOW_REQ:{

                *stCommand=(struct r_app_del*)malloc(sizeof(struct r_app_del));
                
                ((struct r_app_del*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppDelSubFlowReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;
            case MESSAGE_TYPE_TRY_SUBFLOW_REQ:{
                
                 *stCommand=(struct r_app_try*)malloc(sizeof(struct r_app_try));
                
                ((struct r_app_try*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppTrySubFlowReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;
                    
            case MESSAGE_TYPE_KEEPALIVE_REQ:{
                 *stCommand=(struct r_app_mpc_keepalive*)malloc(sizeof(struct r_app_mpc_keepalive));
                
                ((struct r_app_mpc_keepalive*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppMPCKeepAliveReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;
            case MESSAGE_TYPE_ENABLE_MPC_LOG:{
                 *stCommand=(struct r_app_enable_log*)malloc(sizeof(struct r_app_enable_log));
                
                ((struct r_app_enable_log*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppEnablelogReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                    }
                }
                break;

                 
             case MESSAGE_TYPE_RELEASE_MP_REQ:{
               
                *stCommand=(struct r_app_release_mp*)malloc(sizeof(struct r_app_release_mp));                
                ((struct r_app_release_mp*)*stCommand)->baseInfo.messageType = messageType;
                if(decodeAppReleaseMpReq(*stCommand,buffer,len))
                     return;
                else{
                    free(*stCommand);
                    *stCommand = NULL;
                }
             }
                break;
             case MESSAGE_TYPE_SYNC_MP_INIT_REQ:{
               
             }
                break;
            default:
                break;
        }
    }
    return ;

}


bool decodeMagCommandBuffer(struct magRsp* rsp,char * buffer,int len,short netType){


    if((len!=sizeof(struct magRsp)&&len!=sizeof(struct magRsp)+4)||rsp==NULL)
        return false;
    rsp->messageType = *(int *)buffer;
    rsp->sessionId= *(int *)(buffer+4); 
    rsp->len= *(int *)(buffer+8);    

    rsp->messageType = ntohl(rsp->messageType);
    rsp->sessionId = ntohl(rsp->sessionId);     
    rsp->len  = ntohl(rsp->len);    


    rsp->result= *(BYTE *)(buffer+12);


    
    if(len == LEN_TYPE_ADD_SUBFLOW_RSP&&rsp->messageType == MESSAGE_TYPE_ADD_SUBFLOW_RSP && rsp->result == RESULT_OK){

        int ip_value = ntohl(*(int *)(buffer+13));
        char _ip[16];
        sprintf(_ip,"%d.%d.%d.%d", 
            ((ip_value & 0xff000000) >> 24),
            ((ip_value & 0x00ff0000) >> 16),
            ((ip_value & 0x0000ff00) >> 8),
            (ip_value & 0x000000ff));  
        mpc_op.localIp = strdup(_ip);       
    }else if(len == LEN_TYPE_TRY_SUBFLOW_RSP&&rsp->messageType ==MESSAGE_TYPE_TRY_SUBFLOW_RSP){
        unsigned long timeStamp = ntohl(*(unsigned long *)(buffer+13));
        
        int index  = netType/2;
        if(index<3)
            mpc_host.flow_name_status_key[index].RTTC = getTimeStamp()-timeStamp;
    }

    return true;   
}


void *encodeMPSyncInitRequest(struct r_sync_mp_init* r_sync){

    __be32 req_msg_length = LEN_TYPE_SYNC_MP_INIT_REQ;

    u_char* buffer = (u_char*)malloc(req_msg_length);
    memset(buffer, 0x00, req_msg_length); 

    r_sync->baseInfo.messageType = htonl(r_sync->baseInfo.messageType);   
    r_sync->baseInfo.sessionId = htonl(r_sync->baseInfo.sessionId);
       
    req_msg_length = htonl(req_msg_length);

    memcpy(buffer,(u_char*)&(r_sync->baseInfo),sizeof(struct r_base));
    memcpy(buffer+8,&req_msg_length,4);
    int _uid = htonl(mpc_op.uid);
    memcpy(buffer+12,&_uid,4);

    memset(buffer+16,r_sync->numberType,1);
  
    return buffer;


}


void *encodeTryRequest(struct r_app_try* r_try,unsigned short sub_ut_TTl){

    __be32 req_msg_length = LEN_TYPE_TRY_SUBFLOW_REQ;

    u_char* buffer = (u_char*)malloc(req_msg_length);
    memset(buffer, 0x00, req_msg_length); 

    r_try->baseInfo.messageType = htonl(r_try->baseInfo.messageType);

   
    r_try->baseInfo.sessionId = htonl(r_try->baseInfo.sessionId);

       
    req_msg_length = htonl(req_msg_length);

    memcpy(buffer,(u_char*)&(r_try->baseInfo),sizeof(struct r_base));
    memcpy(buffer+8,&req_msg_length,4);
    int _uid = htonl(mpc_op.uid);
    memcpy(buffer+12,&_uid,4);

   

    unsigned long currentTimeStamp  = htonl(getTimeStamp());

    memcpy(buffer+16,&currentTimeStamp,4);

    unsigned short ttl = htons(sub_ut_TTl);

    memcpy(buffer+20,&ttl,2);
    
    return buffer;

}


void *encodeMPReleaseRequest(struct r_app_release_mp* r_release){
  
    __be32 req_msg_length = LEN_TYPE_RELEASE_MP_REQ;

    u_char* buffer = (u_char*)malloc(req_msg_length);
    memset(buffer, 0x00, req_msg_length); 


    r_release->baseInfo.messageType = htonl(r_release->baseInfo.messageType);
    r_release->baseInfo.sessionId = htonl(r_release->baseInfo.sessionId);
    r_release->token= htonl(r_release->token);
    req_msg_length = htonl(req_msg_length);

    memcpy(buffer,(u_char*)&(r_release->baseInfo),sizeof(struct r_base));
    memcpy(buffer+8,&req_msg_length,4);
    int _uid = htonl(mpc_op.uid);
    memcpy(buffer+12,&_uid,4);
    memcpy(buffer+16,&r_release->token,4);
    

    
    memset(buffer+20,r_release->numberType,1);
    memset(buffer+21,r_release->deleteCause,1);
    
    return buffer;   

}

void *encodeDelRequest(struct r_app_del* r_del){

    __be32 req_msg_length = LEN_TYPE_DEL_SUBFLOW_REQ;

    u_char* buffer = (u_char*)malloc(req_msg_length);
    memset(buffer, 0x00, req_msg_length); 


    r_del->baseInfo.messageType = htonl(r_del->baseInfo.messageType);
    r_del->baseInfo.sessionId = htonl(r_del->baseInfo.sessionId);
    r_del->token= htonl(r_del->token);
    req_msg_length = htonl(req_msg_length);

    memcpy(buffer,(u_char*)&(r_del->baseInfo),sizeof(struct r_base));
    memcpy(buffer+8,&req_msg_length,4);
    int _uid = htonl(mpc_op.uid);
    memcpy(buffer+12,&_uid,4);
    memcpy(buffer+16,&r_del->token,4);

    memset(buffer+20,r_del->numberType,1);
    memset(buffer+21,r_del->deleteCause,1);
 
    return buffer;

}


void *encodeAddRequest(struct r_app_add* r_add){

    __be32 req_msg_length = LEN_TYPE_ADD_SUBFLOW_REQ;

    u_char* buffer = (u_char*)malloc(req_msg_length);
    memset(buffer, 0x00, req_msg_length); 
 

   
    int temp ;
    r_add->baseInfo.messageType = htonl(r_add->baseInfo.messageType);
    r_add->baseInfo.sessionId = htonl(r_add->baseInfo.sessionId);
    memcpy(buffer,(u_char*)&(r_add->baseInfo),sizeof(struct r_base));
    
    temp = htonl(req_msg_length);
    memcpy(buffer+8,&temp,4);   
    temp = htonl(mpc_op.uid);
    memcpy(buffer+12,&temp,4);
    temp = htonl(r_add->token);
    memcpy(buffer+16,&temp,4);
    
    memset(buffer+20,r_add->netType,1);
    temp = htonl(r_add->band);
    memcpy(buffer+21,&temp,4);


    
    return buffer;




}


int convertIpToInt(char * ip){
    
    char *ipstr=NULL;  
    char str_ip_index[4]={'\0'};  
    unsigned int ip_int,ip_add=0,ip_int_index[4],ip_temp_numbr=24;  
    int j =0,a=3;     
    unsigned int i =0;
    for(i =0;i<=strlen(ip);i++) 
    {  
        if (ip[i]=='\0'||ip[i]=='.')  
        {             
            ip_int =atoi(str_ip_index);   
            if (ip_int<0||ip_int>255)  
            {  
                printf("Ip error\n");  
                MPCLOG(LLV_INFO, "Ip error\n");
                 
                return 0;                 
            }  
  
            ip_add+=(ip_int*((unsigned int)pow(256.0,a)));            
            a--;  
            memset(str_ip_index,0,sizeof(str_ip_index));  

            j=0;  
            continue;  
        }  
  
        str_ip_index[j]=ip[i];  
        j++;  
    }     
    
    for(j=0;j<4;j++)  
    {  
        ip_int_index[j]=(ip_add>>ip_temp_numbr)&0xFF;  
        ip_temp_numbr-=8;  
    }  
  
    if ((ipstr=(char *)malloc(17*sizeof(char)))==NULL)  
    {  
        return ip_add;  
    }  
      
    sprintf(ipstr,"%d.%d.%d.%d",ip_int_index[0],ip_int_index[1],ip_int_index[2],ip_int_index[3]);  
    
    
    free(ipstr);  
    ipstr=NULL;  
    
    return ip_add;
}

u_char *encodeAppRsp(__be32 messageType,__be32 sessionId,__be32 result){

    char * format = "[{\"messageType\":%d,\"sessionid\":%d,\"result\":%d}]";
    
    u_char* rsp = (u_char*)malloc(80);
    memset(rsp, 0x00, 80);

    sprintf(rsp,format,messageType,sessionId,result);
    

    return rsp;
    
}

u_char *encodeAppTypeAndSessionId(__be32 messageType,__be32 sessionId){
    MPCLOG(LLV_ERROR,"fly %s start",__func__);
    char * format = "[{\"messageType\":%d,\"sessionid\":%d}]";
    u_char* rsp = (u_char*)malloc(80);
    memset(rsp, 0x00, 80);

    sprintf(rsp,format,messageType,sessionId);
    MPCLOG(LLV_ERROR,"fly %s end",__func__);
    return rsp;
    
}

u_char *encodeExceptionRsp(__be32 messageType,__be32 exceptioncode,__be32 netType){

    char * format = "[{\"messageType\":%d,\"exceptionCode\":%d,\"netType\":%d}]";
    u_char* rsp = (u_char*)malloc(80);
    memset(rsp, 0x00,80);

    sprintf(rsp,format,messageType,exceptioncode,netType);

    MPCLOG(LLV_INFO, "encodeExceptionRsp <<<<< rsp = %s\n",rsp);

    return rsp;
    
}


u_char *encodeAppRspStatistics(__be32 messageType,__be32 netType,__be64 statistics){

    char * format = "[{\"messageType\":%d,\"netType\":%d,\"statistics\":%llu}]";
    u_char* rsp = (u_char*)malloc(100);
    memset(rsp, 0x00, 100);

    sprintf(rsp,format,messageType,netType,statistics);

    return rsp;
}


u_char *encodeAppRspWithNetType(__be32 messageType,__be32 sessionId,__be32 netType,__be32 result){

    char * format = "[{\"messageType\":%d,\"sessionid\":%d,\"netType\":%d,\"result\":%d}]";
    u_char* rsp = (u_char*)malloc(100);
    memset(rsp, 0x00, 100);

    sprintf(rsp,format,messageType,sessionId,netType,result);

    return rsp;
    
}


