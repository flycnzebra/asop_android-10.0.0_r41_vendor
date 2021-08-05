//
// Created by FlyZebra on 2021/7/19 0019.
//
#include <binder/IPCThreadState.h>
#include "RtspServer.h"
#include "ScreenDisplay.h"
#include "FlyLog.h"

using namespace android;

int main(int argc, char* const argv[])
{
    FLOGE("rtsp screen is start ...");
    sp<ALooper> looper = new ALooper;
    sp<RtspServer> rtsp = new RtspServer();
    looper->registerHandler(rtsp);
    rtsp->start();
    looper->start(true);
    looper->stop();
    FLOGE("rtsp screen is exit ...");
    return 0;
}

