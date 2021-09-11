//
// Created by FlyZebra on 2021/7/19 0019.
//

#include <binder/IPCThreadState.h>
#include "RtspServer.h"
#include "Controller.h"
#include "FlyLog.h"

using namespace android;

static volatile bool is_stop = false;

static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;

static void signalCatcher(int signum)
{
    FLOGE("signalCatcher %d", signum);
    is_stop = true;
    switch (signum) {
    case SIGINT:
    case SIGHUP:
        sigaction(SIGINT, &gOrigSigactionINT, NULL);
        sigaction(SIGHUP, &gOrigSigactionHUP, NULL);
        break;
    default:
        abort();
        break;
    }
}

static status_t configureSignals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &gOrigSigactionINT) != 0) {
        status_t err = -errno;
        FLOGE("Unable to configure SIGINT handler: %s", strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        status_t err = -errno;
        FLOGE("Unable to configure SIGHUP handler: %s", strerror(errno));
        return err;
    }
    return NO_ERROR;
}

int main(int argc, char* const argv[])
{
    FLOGD("mobilectl start ...");

    is_stop = false;
    status_t err = configureSignals();
    if (err != NO_ERROR) FLOGE("configureSignals failed!");

    androidSetThreadPriority(gettid(), -10);

    //sp<ProcessState> proc(ProcessState::self());
    //ProcessState::self()->startThreadPool();

    sp<android::ALooper> looper_ctrl = new android::ALooper;
    sp<Controller> controller = new Controller();
    looper_ctrl->registerHandler(controller);
    looper_ctrl->start(false);
    controller->start();

    sp<android::ALooper> looper_rtsp = new android::ALooper;
    sp<RtspServer> rtspserver = new RtspServer();
    looper_rtsp->registerHandler(rtspserver);
    looper_rtsp->start(false);
    rtspserver->start();

    while(!is_stop){
        usleep(1000000);
    }

    controller->stop();
    looper_ctrl->unregisterHandler(controller->id());
    looper_ctrl->stop();

    rtspserver->stop();
    looper_rtsp->unregisterHandler(rtspserver->id());
    looper_rtsp->stop();
    FLOGD("mobilectl exit ...");
    return 0;
}

