//
// Created by FlyZebra on 2020/10/22 0022.
//

#ifndef ANDROID_RTSPSERVER_H
#define ANDROID_RTSPSERVER_H

#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include <vector>
#include "ParsedMessage.h"
#include "ScreenDisplay.h"

namespace android {

class RtspServer : public AHandler {
public:
    RtspServer();
    ~RtspServer();
    void start();
protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    static void AppendCommonResponse(AString *response, int32_t cseq);

    void handleStart(const sp<AMessage> &msg);
    void handleClientSocket(const sp<AMessage> &msg);
    void handleSocketRecv(const sp<AMessage> &msg);
    void handleMediaNotify(const sp<AMessage> &msg);
    void handleClientSocketExit(const sp<AMessage> &msg);

    status_t onOptionsRequest(int socket_fd, int32_t cseq);
    status_t onDescribeRequest(int socket_fd, int32_t cseq);
    status_t onSetupRequest(int socket_fd, int32_t cseq);
    status_t onPlayRequest(int socket_fd, int32_t cseq);

	static void *_init_socket(void *arg);
	static void *_client_socket(void *arg);

	void sendSPSPPS(const unsigned char* data, int size, int64_t ptsUsec);
    void sendVFrame(const unsigned char* data, int size, int64_t ptsUsec);

	std::vector<int32_t> thread_sockets;
	std::vector<int32_t> work_sockets;
	std::vector<unsigned char> sps_pps;
	Mutex mLock;
	sp<ScreenDisplay> mScreenDisplay;

	int count = 0;
};

}; // namespace android

#endif //ANDROID_RTSPSERVER_H

