//
// Created by FlyZebra on 2021/7/19 0019.
//
#include <binder/IPCThreadState.h>
#include "RtspServer.h"
#include "Controller.h"
#include "AudioStream.h"
#include "FlyLog.h"

using namespace android;

int main(int argc, char* const argv[])
{
    FLOGE("main start ...");
    sp<ALooper> looper1 = new ALooper;
    sp<Controller> keyevent = new Controller();
    looper1->registerHandler(keyevent);
    keyevent->start();
    looper1->start(false);

    sp<ALooper> looper2 = new ALooper;
    sp<AudioStream> audio = new AudioStream();
    looper2->registerHandler(audio);
    audio->start();
    looper2->start(false);

    sp<ALooper> looper3 = new ALooper;
    sp<RtspServer> rtsp = new RtspServer();
    looper3->registerHandler(rtsp);
    rtsp->start();
    looper3->start(true);

    looper1->stop();
    looper2->stop();
    looper3->stop();
    FLOGE("main stop ...");
    return 0;
}

