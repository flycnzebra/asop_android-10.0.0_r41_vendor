//
// Created by FlyZebra on 2021/7/19 0019.
//
#include <binder/IPCThreadState.h>
#include "RtspServer.h"
#include "FlyLog.h"

using namespace android;

int main(int argc, char* const argv[])
{
    FLOGE("rtsp screen is start ...");
    sp<ALooper> looper = new ALooper;
    sp<RtspServer> rtspServer = new RtspServer();
    looper->registerHandler(rtspServer);
    looper->start();
    rtspServer->start();
    while(true){
        FLOGE("rtsp screen is running ...");
        sleep(60);
    }
    looper->stop();
    FLOGE("rtsp screen is exit ...");
    return 0;
}

