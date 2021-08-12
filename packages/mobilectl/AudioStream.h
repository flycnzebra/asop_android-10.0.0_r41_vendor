//
// Created by FlyZebra on 2021/8/12 0012.
//

#ifndef ANDROID_AUDIOSTREAM_H
#define ANDROID_AUDIOSTREAM_H

#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>

namespace android {

class AudioStream : public AHandler {
public:
    AudioStream();
    ~AudioStream();
    void start();
protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
private:
    static void *_audio_socket(void *arg);
    static void *_audio_client_socket(void *arg);
    void handleClientSocket(const sp<AMessage> &msg);
    void handleSocketRecvData(const sp<AMessage> &msg);
    void handleClientSocketExit(const sp<AMessage> &msg);

    struct client_conn {
        int32_t socket;
        int32_t status;
    };

    Mutex mLock;
    std::vector<int32_t> thread_sockets;
    std::vector<client_conn> conn_sockets;
    bool isStoped = false;
};

}; // namespace android

#endif //ANDROID_AUDIOSTREAM_H
