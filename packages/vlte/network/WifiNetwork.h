//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef ANDROID_WIFINETWORK_H
#define ANDROID_WIFINETWORK_H
#include "network/BaseNetwork.h"

#define CHECK_SIGNAL_TIME 5000

namespace vlte{

class WifiNetwork : public BaseNetwork{
public:
    WifiNetwork();
    ~WifiNetwork();
    int init();
    int openNetwork();
    int closeNetwork();
    int getSignal();
    void release();
    static void *networkSingal(void *argv);
private:
    int rssi;
};


}//namespace vlte


#endif //ANDROID_WIFINETWORK_H
