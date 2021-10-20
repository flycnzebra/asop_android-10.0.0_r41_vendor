//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef ANDROID_USBNETWORK_H
#define ANDROID_USBNETWORK_H
#include "network/BaseNetwork.h"

namespace vlte{

class UsbNetwork : public BaseNetwork{
public:
    UsbNetwork();
    ~UsbNetwork();
    int init();
    int openNetwork();
    int closeNetwork();
    int getSignal();
    void release();
    static void *networkSingal(void *argv);
private:
    bool rndisIsOpen(int maxwait);
private:
    const char *PROP_USB_CONFIG = "sys.usb.config";
    const char *PROR_USB_STATE = "sys.usb.state";
    const char *SET_USB_CONFIG = "rndis,adb";
    const char *GET_USB_CONFIG = "rndis,adb";
};

}//namespace vlte



#endif //ANDROID_USBNETWORK_H
