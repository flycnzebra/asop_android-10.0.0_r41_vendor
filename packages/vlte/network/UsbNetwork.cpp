//
// Created by FlyZebra on 2020/7/20 0020.
//

#include "UsbNetwork.h"
#include "utils/dhcp_utils.h"
#include <string.h>

namespace vlte{

UsbNetwork::UsbNetwork(){
}

UsbNetwork::~UsbNetwork(){
}

bool UsbNetwork::rndisIsOpen(int maxwait)
{
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    while (maxwait>= 0) {
       if (property_get(PROR_USB_STATE, value, NULL)) {
           if (strcmp(value, GET_USB_CONFIG) == 0) {
               return true;
           }
       }
       if(maxwait>0){
           sleep(1);
       }
       maxwait--;
    }
    return false;
}

int UsbNetwork::init(){
    LOGD("usb network init...");
    running = 1;
    pthread_t pid;
    pthread_create(&pid,NULL,networkSingal,(void *)this);
    pthread_detach(pid);
    return 0;
}

int UsbNetwork::openNetwork(){
    LOGD("vlte open usbnetwork...");
    property_set(PROP_STATUS, "start");
    network_status = 0;
    if(!rndisIsOpen(0)){
         property_set(PROP_USB_CONFIG, SET_USB_CONFIG);
    }
    if(!rndisIsOpen(5)){
        LOGD("usbnetwork set usb to ridns error...");
        return -51;
    }
    runcommand("ip link set rndis0 name rmnet_data9",cmdresult);
    runcommand("ip link set rmnet_data9 up",cmdresult);
    if(property_get(PROP_IP, ip, NULL)&&property_get(PROP_MASK, mask, NULL)&&property_get(PROP_GATEWAY, gateway, NULL)){
        LOGD("set with static ip");
        saveStaticDhcp();
        memset(tempbuff,'\0',strlen(tempbuff));
        sprintf(tempbuff,"ndc interface setcfg rmnet_data9 %s %d",ip,netmask_str2len(mask));
        runcommand(tempbuff,cmdresult);
        if(strncmp(cmdresult, "200", 3) != 0){
            LOGD("set static ip error will use dhcp");
            if(dhcp_start()!=0){
                return -8;
            }
            LOGD("dhcp request ok");
        }
    }else{
        LOGD("start request dhcp");
        if(dhcp_start()!=0){
            return -8;
        }
        LOGD("dhcp request ok");
    }
    property_set(PROP_STATUS, "connect");
    network_status = 1;
    return 0;
}

int UsbNetwork::closeNetwork(){
    LOGD("vlte close usbnetwork...");
    runcommand("ip link set rmnet_data9 down",cmdresult);
    runcommand("ip link set rmnet_data9 name rndis0",cmdresult);
    property_set(PROP_STATUS, "stop");
    network_status = -1;
    property_set("dhcp.rmnet_data9.dns1", "");
    property_set("dhcp.rmnet_data9.dns2", "");
    property_set("dhcp.rmnet_data9.dns3", "");
    property_set("dhcp.rmnet_data9.dns4", "");
    property_set("dhcp.rmnet_data9.domain", "");
    property_set("dhcp.rmnet_data9.gateway", "");
    property_set("dhcp.rmnet_data9.ipaddress", "");
    property_set("dhcp.rmnet_data9.leasetime", "");
    property_set("dhcp.rmnet_data9.mask", "");
    property_set("dhcp.rmnet_data9.mtu", "");
    property_set("dhcp.rmnet_data9.pid", "");
    property_set("dhcp.rmnet_data9.reason", "");
    property_set("dhcp.rmnet_data9.result", "");
    property_set("dhcp.rmnet_data9.server", "");
    property_set("dhcp.rmnet_data9.vendorInfo", "");
    //property_set(PROP_USB_CONFIG, "rndis,none,adb");
    LOGD("close network finish!");
    return 0;
}

void UsbNetwork::release(){
    running = 0;
}

int UsbNetwork::getSignal(){
    return 25;
}

void *UsbNetwork::networkSingal(void *argv){
    UsbNetwork *network=(UsbNetwork *)argv;
    while(network->running){
        //LOGD("refresh signal...");
        //memset(network->threadStr,'\0',strlen(network->threadStr));
        //int signal = network->getSignal();
        //sprintf(network->threadStr,"[{SIGNAL:%d}]",signal);
        //network->sendTo(network->java_fd,network->threadStr);
        sleep(15);
    }
    return 0;
}

}//namespace vlte