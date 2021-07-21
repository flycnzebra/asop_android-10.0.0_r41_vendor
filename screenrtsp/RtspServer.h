//
// Created by FlyZebra on 2020/10/22 0022.
//

#ifndef ANDROID_RTSPSERVER_H
#define ANDROID_RTSPSERVER_H

#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>

namespace android {

class RtspServer : public AHandler {
public:
    RtspServer();
    ~RtspServer();
    void network();
    status_t start();
    void sendVideoFrame();
protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
private:
    enum {
        kWhatStart,
        kWhatRTSPNotify,
        kWhatStop,
        kWhatPause,
        kWhatResume,
        kWhatReapDeadClients,
        kWhatPlaybackSessionNotify,
        kWhatKeepAlive,
        kWhatHDCPNotify,
        kWhatFinishStop2,
        kWhatTeardownTriggerTimedOut,
    };
};

}; // namespace android

#endif //ANDROID_RTSPSERVER_H

