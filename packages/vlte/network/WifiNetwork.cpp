//
// Created by FlyZebra on 2020/7/20 0020.
//

#include <regex.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <string.h>
#include "network/WifiNetwork.h"
#include "utils/FlyLog.h"
#include "utils/wifi.h"
#include "utils/dhcp_utils.h"

namespace vlte{

WifiNetwork::WifiNetwork(){
}

WifiNetwork::~WifiNetwork(){
}

int WifiNetwork::init(){
    //wlan.ko
    LOGD("vlte network init...");
    system("/system/bin/insmod /system/lib/modules/wlan.ko");
    system("/system/bin/ip link set wlan0 name rmnet_data9");
    system("/system/bin/ip link set rmnet_data9 up");
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
    running = 1;
    pthread_t pid;
    pthread_create(&pid,NULL,networkSingal,(void *)this);
    pthread_detach(pid);
    return 0;
}

int WifiNetwork::openNetwork(){
    LOGD("vlte open wifinetwork...");
    property_set(PROP_STATUS, "start");
    network_status = 0;
    LOGD("load driver ok");
     //set wifi STA
    const char* fwpath = wifi_get_fw_path(2);
    if (wifi_change_fw_path(fwpath) != 0) {
        LOGE("switch sta error!");
    }else{
        LOGD("switch sta ok!");
    }

    if(wifi_start_supplicant(0)!=0){
        //system("/system/bin/ifconfig rmnet_data9 down");
        //system("/system/bin/ip link set rmnet_data9 name wlan0");
        return -1;
    }
    LOGD("start supplicant ok");
    LOGD("will connect wifi");
    for( int i = 0; i < 3; i++ ){
        runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets add_network",cmdresult);
        int id;
        if (sscanf(cmdresult, "%d", &id) == 1){
            if(id>0){
                memset(tempbuff,'\0',strlen(tempbuff));
                sprintf(tempbuff,"/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets remove_network %d",id);
                runcommand(tempbuff,cmdresult);
            }
            break;
        }else{
            LOGD("add_network failed");
            sleep(1);
        }
        if(i==2){
            wifi_stop_supplicant(0);
            system("/system/bin/ndc interface clearaddrs rmnet_data9");
            system("/system/bin/ndc interface setcfg rmnet_data9 0.0.0.0 0");
            system("/system/bin/ndc interface clearaddrs rmnet_data9");
            //system("/system/bin/ifconfig rmnet_data9 down");
            //system("/system/bin/ip link set rmnet_data9 name wlan0");
            return -2;
        }
    }
    if(property_get(PROP_SSID, ssid, "Xiaomi_zd_5G")){
        memset(tempbuff,'\0',strlen(tempbuff));
        sprintf(tempbuff,"/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets set_network 0 ssid '\"%s\"'",ssid);
        runcommand(tempbuff,cmdresult);
        if(strncmp(cmdresult, "OK", 2) != 0){
            wifi_stop_supplicant(0);
            //system("/system/bin/ifconfig rmnet_data9 down");
            //system("/system/bin/ip link set rmnet_data9 name wlan0");
            return -3;
        }
    }
    if(property_get(PROP_PSK, psk, "szlj@2017")){
        memset(tempbuff,'\0',strlen(tempbuff));
        sprintf(tempbuff,"/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets set_network 0 psk '\"%s\"'",psk);
        runcommand(tempbuff,cmdresult);
        if(strncmp(cmdresult, "OK", 2) != 0){
            wifi_stop_supplicant(0);
            //system("/system/bin/ifconfig rmnet_data9 down");
            //system("/system/bin/ip link set rmnet_data9 name wlan0");
            return -4;
        }
    }else {
        runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets set_network 0 key_mgmt NONE",cmdresult);
        runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets set_network 0 scan_ssid 1",cmdresult);
        if (strncmp(cmdresult, "OK", 2) != 0) {
            return -4;;
        }
    }
    runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets enable_network 0",cmdresult);
    if(strncmp(cmdresult, "OK", 2) != 0){
        wifi_stop_supplicant(0);
        //system("/system/bin/ifconfig rmnet_data9 down");
        //system("/system/bin/ip link set rmnet_data9 name wlan0");
        return -5;
    }
    runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets select_network 0",cmdresult);
    if(strncmp(cmdresult, "OK", 2) != 0){
        wifi_stop_supplicant(0);
        //system("/system/bin/ifconfig rmnet_data9 down");
        //system("/system/bin/ip link set rmnet_data9 name wlan0");
        return -6;
    }
    for( int i = 0; i < 15; i++ ){
        runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets signal_poll",cmdresult);
        int rssi;
        if(sscanf(cmdresult, "RSSI=%d", &rssi) == 1){
            if(rssi>-127)
            break;
        }
        sleep(1);
        if(i==14){
            wifi_stop_supplicant(0);
            //system("/system/bin/ifconfig rmnet_data9 down");
            //system("/system/bin/ip link set rmnet_data9 name wlan0");
            return -7;
        }
    }
    if(property_get(PROP_IP, ip, NULL)&&property_get(PROP_MASK, mask, NULL)&&property_get(PROP_GATEWAY, gateway, NULL)){
        LOGD("set with static ip");
        saveStaticDhcp();
        memset(tempbuff,'\0',strlen(tempbuff));
        sprintf(tempbuff,"ndc interface setcfg rmnet_data9 %s %d",ip,netmask_str2len(mask));
        runcommand(tempbuff,cmdresult);
        if(strncmp(cmdresult, "200", 3) != 0){
            LOGD("set static ip error will use dhcp");
            if(dhcp_start()!=0){
                wifi_stop_supplicant(0);
                //system("/system/bin/ifconfig rmnet_data9 down");
                //system("/system/bin/ip link set rmnet_data9 name wlan0");
                return -8;
            }
            LOGD("dhcp request ok");
        }
        //memset(tempbuff,'\0',strlen(tempbuff));
        //sprintf(tempbuff,"ip route add default via %s dev rmnet_data9",gateway);
        //runcommand(tempbuff,cmdresult);
        //memset(tempbuff,'\0',strlen(tempbuff));
        //sprintf(tempbuff,"ip route add default via %s dev rmnet_data9 table rmnet_data9",gateway);
        //runcommand(tempbuff,cmdresult);
    }else{
        LOGD("start request dhcp");
        if(dhcp_start()!=0){
            wifi_stop_supplicant(0);
            //system("/system/bin/ifconfig rmnet_data9 down");
            //system("/system/bin/ip link set rmnet_data9 name wlan0");
            return -8;
        }
        //property_get("dhcp.rmnet_data9.gateway", gateway, NULL);
        //memset(tempbuff,'\0',strlen(tempbuff));
        //sprintf(tempbuff,"ip route add default via %s dev rmnet_data9",gateway);
        //runcommand(tempbuff,cmdresult);
        //memset(tempbuff,'\0',strlen(tempbuff));
        //sprintf(tempbuff,"ip route add default via %s dev rmnet_data9 table rmnet_data9",gateway);
        //runcommand(tempbuff,cmdresult);
        LOGD("dhcp request ok");
    }
    property_set(PROP_STATUS, "connect");
    network_status = 1;
    return 0;
}

int WifiNetwork::closeNetwork(){
    LOGD("vlte close wifinetwork...");
    property_set(PROP_STATUS, "stop");
    network_status = -1;
    runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets disable_network 0",cmdresult);
    runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets disconnect",cmdresult);
    runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets remove_network 0",cmdresult);
    if(wifi_stop_supplicant(0)!=0){
        LOGE("wifi_stop_supplicant error...");
    }
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
    return 0;
}

void WifiNetwork::release(){
    system("/system/bin/ifconfig rmnet_data9 down");
    system("/system/bin/ip link set rmnet_data9 name wlan0");
    system("/system/bin/ip link set rmnet_data9 name wlan0");
    system("/system/bin/rmmod /system/lib/modules/wlan.ko");
    running = 0;
}

int WifiNetwork::getSignal(){
    return rssi;
}

void *WifiNetwork::networkSingal(void *argv){
    WifiNetwork *network=(WifiNetwork *)argv;
    while(network->running){
        if(network->network_status == 1 ){
            network->runcommand("/vendor/bin/wpa_cli -irmnet_data9 -p/data/misc/wifi/sockets signal_poll",network->t_cmdresult);
            if(sscanf(network->t_cmdresult, "RSSI=%d", &network->rssi) == 1){
                memset(network->threadStr,'\0',strlen(network->threadStr));
                int signal = network->getSignal();
                sprintf(network->threadStr,"[{SIGNAL:%d}]",signal);
                network->sendTo(network->java_fd,network->threadStr);
                if(network->rssi<=-127){
                    //TODO:
                }
            }
        }
        sleep(10);
    }
    return 0;
}

}//namespace vlte