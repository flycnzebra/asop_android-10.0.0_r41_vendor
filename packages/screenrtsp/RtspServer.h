//
// Created by FlyZebra on 2020/10/22 0022.
//

#ifndef ANDROID_RTSPSERVER_H
#define ANDROID_RTSPSERVER_H

#include <binder/IPCThreadState.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include <vector>
#include <arpa/inet.h>
#include "ScreenDisplay.h"

namespace android {

enum {
    RTP_TCP,
    RTP_UDP,
};

enum {
    S_SETUP,
    S_PLAY,
};

struct client_connect {
      int32_t socket;
      int32_t type;
      int32_t rtp_port;
      int32_t rtcp_port;
      struct sockaddr_in addr_in;
      socklen_t addrLen;
      int32_t status;
};

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

    status_t onOptionsRequest(const char* data, int32_t socket_fd, int32_t cseq);
    status_t onDescribeRequest(const char* data, int32_t socket_fd, int32_t cseq);
    status_t onSetupRequest(const char* data, int32_t socket_fd, int32_t cseq);
    status_t onPlayRequest(const char* data, int32_t socket_fd, int32_t cseq);
    status_t onGetParameterRequest(const char* data, int32_t socket_fd, int32_t cseq);
    status_t onOtherRequest(const char* data, int32_t socket_fd, int32_t cseq);

	static void *_server_socket(void *arg);
	static void *_client_socket(void *arg);
	static void *_rtpudp_socket(void *arg);
	static void *_rtcpudp_socket(void *arg);

	void sendSPSPPS(const unsigned char* data, int32_t size, int64_t ptsUsec);
    void sendVFrame(const unsigned char* data, int32_t size, int64_t ptsUsec);

	Mutex mLock;
	std::vector<int32_t> thread_sockets;
	std::vector<client_connect> work_sockets;
	std::vector<unsigned char> sps_pps;
	sp<ScreenDisplay> mScreenDisplay;

	int32_t sequencenumber = 0;
	int32_t rtp_socket;
	int32_t rtcp_socket;
	
	bool isStoped = false;
};

}; // namespace android

#endif //ANDROID_RTSPSERVER_H

