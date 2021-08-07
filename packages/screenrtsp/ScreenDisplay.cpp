//
// Created by FlyZebra on 2021/7/30 0030.
//

#include "ScreenDisplay.h"
#include "HandlerEvent.h"
#include "FlyLog.h"
#include "screenrecord.h"

using namespace android;

ScreenDisplay::ScreenDisplay(sp<AMessage> notify)
:mNotify(notify){
}

ScreenDisplay::~ScreenDisplay(){
}

void ScreenDisplay::startRecord(){
    FLOGE("startRecord!");
    if(isRunning){
        FLOGE("ScreenDisplay is running, exit!");
        return;
    }
    pthread_t run_tid;
    int ret = pthread_create(&run_tid, nullptr, _run_record, (void *) this);
    if (ret != 0) {
    	FLOGE("create socket thread error!");
    	exit(-1);
    }
}

void *ScreenDisplay::_run_record(void *argv){
    FLOGE("_run_record start!");
    auto *p=(ScreenDisplay *)argv;
    p->isRunning = true;
    screenrecord_start(p->mNotify);
    p->isRunning = false;
    FLOGE("_run_record exit!");
    return 0;
}

void ScreenDisplay::stopRecord(){
    FLOGE("stopRecord!");
    screenrecord_stop();
    usleep(100000);
    while(isRunning){
        FLOGE("stopRecord -> wait run thread exit!");
        usleep(100000);
    }
}