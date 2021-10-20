//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef ANDROID_BASENETWORK_H
#define ANDROID_BASENETWORK_H

#include <cutils/properties.h>
#include "utils/FlyLog.h"

namespace vlte{

class BaseNetwork{
public:
    BaseNetwork();
    virtual ~BaseNetwork(){}
    virtual int init()=0;
    virtual int openNetwork()=0;
    virtual int closeNetwork()=0;
    virtual void release(){}
    virtual int getSignal()=0;

    int netmask_str2len(char* mask);
    void runcommand(const char *cmd, char *buffer);
    void setJavaFD(int fd);
    void setAtFD(int fd);
    void sendTo(int fd, char *message);
    void saveStaticDhcp();

protected:
    int java_fd;
    int at_fd;
    int network_status;
    int running;

    const char *PROP_SSID    = "persist.sys.vlte.ssid";
    const char *PROP_PSK     = "persist.sys.vlte.psk";
    const char *PROP_IP      = "persist.sys.vlte.ip";
    const char *PROP_MASK    = "persist.sys.vlte.mask";
    const char *PROP_GATEWAY = "persist.sys.vlte.gateway";
    const char *PROP_DNS1    = "persist.sys.vlte.dns1";
    const char *PROP_DNS2    = "persist.sys.vlte.dns2";
    const char *PROP_STATUS  = "persist.sys.vlte.status";
    const char *PROP_VLTE    = "persist.sys.vlte.switch";
    const char *VER1         = "ro.build.version.incremental";
    const char *VER2         = "ro.redstone.version";
    char tempbuff[4096];
    char cmdresult[4096];
    char t_cmdresult[4096];
    char tempstr[4096];
    char ssid[PROPERTY_VALUE_MAX];
    char psk[PROPERTY_VALUE_MAX];
    char ip[PROPERTY_VALUE_MAX];
    char mask[PROPERTY_VALUE_MAX];
    char gateway[PROPERTY_VALUE_MAX];
    char dns1[PROPERTY_VALUE_MAX];
    char dns2[PROPERTY_VALUE_MAX];
    char threadStr[64];
};

}//namespace vlte

#endif //ANDROID_BASENETWORK_H
