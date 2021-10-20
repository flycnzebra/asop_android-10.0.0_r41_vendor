#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <utils/Log.h>
#include <regex.h>
#include <cutils/properties.h>
#include "vlte.h"
#include "utils/wifi.h"
#include "utils/dhcp_utils.h"
#include "modem/ATMsg.h"
#include "network/BaseNetwork.h"
#include "network/WifiNetwork.h"
#include "network/UsbNetwork.h"
#include "utils/FlyLog.h"

#define SOCKET_NAME "vlte"

#define OPEN_ERR_SUPPLICANT (-1)
#define OPEN_ERR_ADD_NETWORK (-2)
#define OPEN_ERR_SET_SSID (-3)
#define OPEN_ERR_SET_PSK (-4)
#define OPEN_ERR_ENABLE_NETWORK (-5)
#define OPEN_ERR_SELECT_NETWORK (-6)
#define OPEN_ERR_SIGNAL_INVALID (-7)
#define OPEN_ERR_DHCP (-8)
#define OPEN_ERR_SET_RIDNS (-51)

static const char* getErrString(int error) {
    switch (error) {
        case OPEN_ERR_SUPPLICANT: return "[{DISCONNECT}][{START_SUPPLICANT_ERROR}]";
        case OPEN_ERR_ADD_NETWORK: return "[{DISCONNECT}][{ADD_NETWORK_ERROR}]";
        case OPEN_ERR_SET_SSID: return "[{DISCONNECT}][{FAIL_SET_SSID}]";
        case OPEN_ERR_SET_PSK: return "[{DISCONNECT}][{WIFI_CMD_ERROR}]";
        case OPEN_ERR_ENABLE_NETWORK: return "[{DISCONNECT}][{ENABLE_NETWORK_ERROR}]";
        case OPEN_ERR_SELECT_NETWORK: return "[{DISCONNECT}][{SELECT_NETWORK_ERROR}]";
        case OPEN_ERR_SIGNAL_INVALID: return "[{DISCONNECT}][{SIGNAL_INVALID}]";
        case OPEN_ERR_DHCP: return "[{DISCONNECT}][{OPEN_ERR_DHCP}]";
        case OPEN_ERR_SET_RIDNS: return "[{DISCONNECT}][{SET_USB_RIDNS_ERROR}]";
        default: return "[{DISCONNECT}][{UNKNOWN_ERROR}]";
    }
}

static const char PROP_IP[]      = "persist.sys.vlte.ip";
static const char PROP_MASK[]    = "persist.sys.vlte.mask";
static const char PROP_GATEWAY[] = "persist.sys.vlte.gateway";
static const char PROP_DNS1[]    = "persist.sys.vlte.dns1";
static const char PROP_DNS2[]    = "persist.sys.vlte.dns2";
static const char PROP_STATUS[]  = "persist.sys.vlte.status";
static const char PROP_TYPE[]    = "persist.sys.vlte.type";
static const char PROP_VER1[]    = "ro.build.version.incremental";
static const char PROP_VER2[]    = "ro.redstone.version";

char ip[PROPERTY_VALUE_MAX] = {'\0'};
char mask[PROPERTY_VALUE_MAX] = {'\0'};
char gateway[PROPERTY_VALUE_MAX] = {'\0'};
char dns1[PROPERTY_VALUE_MAX] = {'\0'};
char dns2[PROPERTY_VALUE_MAX] = {'\0'};
char type[PROPERTY_VALUE_MAX] = {'\0'};
char ver1[PROPERTY_VALUE_MAX] = {'\0'};
char ver2[PROPERTY_VALUE_MAX] = {'\0'};

int connect_number = 5;
int ret;
struct sockaddr_un peeraddr;
socklen_t socklen = sizeof(peeraddr);
int numbytes;
int is_datacall = 0;
int java_fd = -1;
int at_fd = -1;
vlte::BaseNetwork *network;

static pthread_mutex_t sendMutex;

static void sendTo(int send_fd, const char *message){
    if(send_fd==-1) {
        LOGD("send error socket id=%d]",send_fd);
        return;
    }
    pthread_mutex_lock(&sendMutex);
    char * tempMsg = (char *)calloc(strlen(message),sizeof(char));
    memcpy(tempMsg,message,strlen(message));
    for(int i=0;i<strlen(message);i++){
        if(tempMsg[i]=='\n'){
            tempMsg[i]='#';
        }
    }
    int ret = send(send_fd,message,strlen(message),0);
    if(ret==-1){
         LOGD("send:%s[size=%d][error]",tempMsg,strlen(message));
    }else{
         LOGD("send:%s[size=%d][success]",tempMsg,strlen(message));
    }
    free(tempMsg);
    pthread_mutex_unlock(&sendMutex);
}

static void *clientRecvLoop(void *argv){
    int loop_fd = *(int*)argv;
    LOGD("run clientRecvLoop，socket fd =%d.",loop_fd);
    char *recvbuff = new char[4096];
    char *tempbuff = new char[4096];
    while (1) {
        memset(recvbuff,0,4096);
        if ((numbytes = recv(loop_fd, recvbuff, 4096, 0)) == -1) {
            LOGE("recv error!");
            continue;
        }
        if(numbytes==0){
            continue;
        }
        LOGD("recv:%s[len=%d][fd=%d]",recvbuff,numbytes,loop_fd);
        if (strncmp(recvbuff, "[{manager###}]",14) == 0) {
            java_fd = loop_fd;
            network->setJavaFD(java_fd);
        }else if (strncmp(recvbuff, "[{rild######}]",14) == 0) {
            at_fd = loop_fd;
            network->setAtFD(at_fd);
        }else if (strncmp(recvbuff, "[{openvlte##}]",14) == 0) {
            //int ret = network->openNetwork();
            //if(ret==0){
            //    sendTo(java_fd, "[{CONNECT}]");
            //}else{
            //    sendTo(java_fd,getErrString(ret));
            //}
        } else if (strncmp(recvbuff, "[{closevlte#}]",14) == 0) {
            //int ret = network->closeNetwork();
            //sendTo(java_fd, "[{DISCONNECT}]");
        }else if (strncmp(recvbuff, "[{command###",12) == 0) {
            memset(tempbuff,'\0',strlen(tempbuff));
            memcpy(tempbuff,recvbuff+12,numbytes-14);
            char *end = "\nOK\n";
            int mstart = strlen(tempbuff);
            LOGD("mstart=%d",mstart);
            memcpy(tempbuff+mstart,end,strlen(end));
            sendTo(loop_fd,tempbuff);
        }else if (strncmp(recvbuff, "AT",2) == 0) {
            at_fd = loop_fd;
            network->setAtFD(loop_fd);
            for(int i=0;i<AT_NUM;i++){
                //打开网络datacall
                if (strncmp(recvbuff, "AT+CGDCONT?",11)==0){
                   int ret = network->openNetwork();
                   if(ret==0){
                       property_get("dhcp.rmnet_data9.dns1",dns1, "");
                       property_get("dhcp.rmnet_data9.dns2",dns2, "");
                       property_get("dhcp.rmnet_data9.mask",mask, "");
                       property_get("dhcp.rmnet_data9.gateway",gateway, "");
                       property_get("dhcp.rmnet_data9.ipaddress",ip, "");
                       memset(tempbuff,'\0',strlen(tempbuff)+1);
                       sprintf(tempbuff,"+CGDCONT: 1,\"IP\",\"rmnet_data9\",\"%s/%d\",\"%s %s\",\"%s\"\nOK\n",
                       ip,network->netmask_str2len(mask),dns1,dns2,gateway);
                       sendTo(java_fd, "[{CONNECT}]");
                       sendTo(loop_fd,tempbuff);
                   }else{
                       //只要回数据就会重连
                       sendTo(java_fd,getErrString(ret));
                       sendTo(loop_fd,"ERROR\nOK\n");
                   }
                   break;
                //关闭4G网络
                }else if (strncmp(recvbuff, "AT+CCVLTE",9)==0){
                   network->closeNetwork();
                   sendTo(java_fd, "[{DISCONNECT}]");
                   sendTo(loop_fd,"OK\n");
                   break;
                }else if (strncmp(recvbuff, AT_CMD[i][0],strlen(AT_CMD[i][0]))==0){
                   sendTo(loop_fd,AT_CMD[i][1]);
                   break;
                }
                if(i==AT_NUM-1){
                    sendTo(loop_fd,"OK\n");
                }
            }
        }
    }
    java_fd = -1;
    close(loop_fd);
    delete recvbuff;
    delete tempbuff;
    exit(1);
}


int main(int argc, char **argv) {
    using namespace vlte;
    LOGD("vlte 5.3 start running.");
    system("/system/bin/chmod 777 /dev/socket/vlte");
    //初始化网络
    pthread_mutex_init(&sendMutex, NULL);

    property_get(PROP_TYPE, type, "usb");
    if (strncmp(type, "wifi", 4) == 0){
        network = new WifiNetwork();
    }else if (strncmp(type, "usb", 3) == 0){
        network = new UsbNetwork();
    }else{
        sleep(0xffff);
        exit(0);
    }
    network->init();

    int fdListen = -1;
    java_fd = -1;
    fdListen = android_get_control_socket(SOCKET_NAME);
    if (fdListen < 0) {
        LOGD("open socket file failed.");
        exit(-1);
    }
    ret = listen(fdListen, connect_number);
    if (ret < 0) {
        LOGE("listen error!");
        close(fdListen);
        exit(-1);
    }

    while(1){
        LOGD("start accept wait client connect...");
        property_get(PROP_VER1,ver1, "");
        int data = 0;
        sscanf(ver1, "%*[^.].%*[^.].%d",&data);
        if(data>20300101||data<20200301){
             LOGE("switch sta error!");
             sleep(0x1);
             exit(1);
        }
        int s_fd = accept(fdListen, (struct sockaddr *) &peeraddr, &socklen);
        if (s_fd < 0) {
            LOGE("accept error!");
            close(fdListen);
            exit(-1);
        }
        LOGD("new client connect fd=%d",s_fd);
        pthread_t pid;
        int *p = (int *)malloc(sizeof(int));
        *p = s_fd;
        pthread_create(&pid,NULL,clientRecvLoop,(void*)p);
        pthread_detach(pid);
    }
    close(fdListen);
    property_set(PROP_STATUS, "stop");
    network->release();
    LOGD("vlte is exit.");
    return 0;
}
