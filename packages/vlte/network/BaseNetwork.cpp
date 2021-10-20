//
// Created by FlyZebra on 2020/7/20 0020.
//

#include "BaseNetwork.h"
#include "WifiNetwork.h"
#include <arpa/inet.h>
#include <string.h>

namespace vlte{

BaseNetwork::BaseNetwork(){
    tempbuff[4096] = {0};
    cmdresult[4096] = {0};
    ssid[PROPERTY_VALUE_MAX] = {0};
    psk[PROPERTY_VALUE_MAX] = {0};
    ip[PROPERTY_VALUE_MAX] = {0};
    mask[PROPERTY_VALUE_MAX] = {0};
    gateway[PROPERTY_VALUE_MAX] = {0};
    dns1[PROPERTY_VALUE_MAX] = {0};
    dns2[PROPERTY_VALUE_MAX] = {0};
    java_fd = -1;
    network_status = 0;
    running = 0;
}

int BaseNetwork::netmask_str2len(char* mask){
    int netmask = 0;
    unsigned int mask_tmp;
    mask_tmp = ntohl((int)inet_addr(mask));
    while (mask_tmp & 0x80000000){
        netmask++;
        mask_tmp = (mask_tmp << 1);
    }
    return netmask;
}

void BaseNetwork::runcommand(const char *cmd, char *buffer){
    memset(buffer,'\0',strlen(buffer)+1);
    char buf_ps[1024];
    char ps[1024]={'\0'};
    FILE *ptr;
    strcpy(ps, cmd);
    if((ptr=popen(ps, "r"))!=NULL){
        while(fgets(buf_ps, 1024, ptr)!=NULL) {
           strcat(buffer, buf_ps);
           if(strlen(buffer)>3072)
               break;
        }
        pclose(ptr);
        ptr = NULL;
        LOGD("run command: %s .result:{%s}", cmd, buffer);
    } else {
        LOGD("run command: %s error", ps);
    }
}

void BaseNetwork::setJavaFD(int fd){
    java_fd = fd;
}

void BaseNetwork::setAtFD(int fd){
    at_fd = fd;
}

void BaseNetwork::sendTo(int fd, char *message){
    if(java_fd==-1) {
        LOGD("send:%s,size[%d][nosend]",message,strlen(message));
        return;
    }
    int ret = send(fd,message,strlen(message),0);
    if(ret==-1){
         LOGD("send:%s,size[%d][error]",message,strlen(message));
    }else{
         LOGD("send:%s,size[%d][success]",message,strlen(message));
    }
}

void BaseNetwork::saveStaticDhcp(){
     property_get(PROP_DNS1,dns1, "");
     property_get(PROP_DNS2,dns2, "");
     property_get(PROP_MASK,mask, "");
     property_get(PROP_GATEWAY,gateway, "");
     property_get(PROP_IP, ip, "");
     property_set("dhcp.rmnet_data9.dns1",dns1);
     property_set("dhcp.rmnet_data9.dns2",dns2);
     property_set("dhcp.rmnet_data9.mask",mask);
     property_set("dhcp.rmnet_data9.gateway",gateway);
     property_set("dhcp.rmnet_data9.ipaddress",ip);
}

}//namespace vlte