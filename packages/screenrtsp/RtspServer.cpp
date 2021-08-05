//
// Created by FlyZebra on 2020/10/22 0022.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include "RtspServer.h"
#include "FlyLog.h"
#include "Base64.h"
#include "HandlerEvent.h"

using namespace android;

RtspServer::RtspServer(){
}

RtspServer::~RtspServer()
{
}

void RtspServer::AppendCommonResponse(AString *response, int32_t cseq) {
    time_t now = time(NULL);
    struct tm *now2 = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);
    response->append("Date: ");
    response->append(buf);
    response->append("\r\n");
    response->append("User-Agent: Android screen rtsp(author zebra)\r\n");
    if (cseq >= 0) {
        char temp[128];
        sprintf(temp, "CSeq: %d\r\n",cseq);
        response->append(temp);
    }
}

void RtspServer::onMessageReceived(const sp<AMessage> &msg){
    switch (msg->what()) {
        case kWhatStart:            
            handleStart(msg);
       		break;
		case kWhatClientSocket:
            handleClientSocket(msg);
			break;
		case kWhatClientSocketExit:
		    handleClientSocketExit(msg);
		    break;
		case kWhatSocketRecvData:
			handleSocketRecv(msg);
			break;
		case kWhatMediaNotify:
			handleMediaNotify(msg);
		    break;
    }
}

void RtspServer::start(){
    FLOGE("RtspServer start");
    sp<AMessage> notify = new AMessage(kWhatMediaNotify, this);
    mScreenDisplay = new ScreenDisplay(notify);
    sp<AMessage> msg = new AMessage(kWhatStart, this);
    msg->post();
}

void *RtspServer::_init_socket(void *argv){
	FLOGE("_init_socket start!");
    int32_t conn_fd = -1;
    int32_t ret = -1;
    int32_t server_ip_port = 554;

    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(server_ip_port);

    int32_t listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        FLOGE("socket error %s errno: %d", strerror(errno), errno);
    }

    ret = bind(listen_fd,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        FLOGE( "bind socket error %s errno: %d", strerror(errno), errno);
    }

    ret = listen(listen_fd, 1024);
    if (ret < 0) {
        FLOGE("listen error %s errno: %d", strerror(errno), errno);
    }

    for(;;) {
        conn_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);
        if(conn_fd < 0) {
            FLOGE("accpet socket error: %s errno :%d", strerror(errno), errno);
            continue;
        }
		sp<AMessage> msg = new AMessage(kWhatClientSocket, (RtspServer *) argv);
		msg->setInt32("socket", conn_fd);
		msg->post();
    }

    close(listen_fd);
    listen_fd = -1;
    FLOGE("_init_socket exit!");
	return 0;
}

void *RtspServer::_client_socket(void *argv){
    FLOGE("_client_socket start!");
    auto *p=(RtspServer *)argv;
    int32_t socket_fd;
    {
	    Mutex::Autolock autoLock(p->mLock);
	    socket_fd = p->thread_sockets.back();
	    FLOGE("client socket_fd=%d", socket_fd);
	    p->thread_sockets.pop_back();
	}
	char recvBuf[1024];
    int32_t recvLen = -1;
	for(;;){
	    memset(recvBuf,0,1024);
	    recvLen = recv(socket_fd, recvBuf, 1024, 0);
	    FLOGE("recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
        if (recvLen <= 0 && errno != 0) {
            sp<AMessage> msg = new AMessage(kWhatClientSocketExit, (RtspServer *) argv);
            msg->setInt32("socket", socket_fd);
            msg->post();
            close(socket_fd);
            break;
        }else if(recvLen > 0){
            size_t length;
            sp<ParsedMessage> data = ParsedMessage::Parse(recvBuf, recvLen, false, &length);
            sp<AMessage> msg = new AMessage(kWhatSocketRecvData, (RtspServer *) argv);
            msg->setObject("data", data);
            msg->setInt32("socket", socket_fd);
            msg->post();
        }
	}
	FLOGE("_client_socket exit!");
	return 0;
}

void RtspServer::handleStart(const sp<AMessage> &msg)
{
    FLOGE("onMessageReceived kWhatStart");
    pthread_t init_socket_tid;
    int ret = pthread_create(&init_socket_tid, nullptr, _init_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create socket thread error!");
    	exit(-1);
    }
}

void RtspServer::handleClientSocket(const sp<AMessage> &msg)
{
    FLOGE("onMessageReceived kWhatClientSocket!");
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
    Mutex::Autolock autoLock(mLock);
    thread_sockets.push_back(socket_fd);
    pthread_t client_socket_tid;
    int ret = pthread_create(&client_socket_tid, nullptr, _client_socket, (void *)this);
    if (ret != 0) {
    	FLOGE("create client socket thread error!");
    	thread_sockets.pop_back();
    }
}

void RtspServer::handleSocketRecv(const sp<AMessage> &msg)
{
    FLOGE("onMessageReceived kWhatClientData!");
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
	sp<RefBase> obj;
    CHECK(msg->findObject("data", &obj));
    sp<ParsedMessage> data = static_cast<ParsedMessage *>(obj.get());
    AString method;
    data->getRequestField(0, &method);
    int32_t cseq;
    data->findInt32("cseq", &cseq);
    if (method == "OPTIONS") {
        onOptionsRequest(socket_fd, cseq);
    }else if(method == "DESCRIBE"){
        onDescribeRequest(socket_fd, cseq);
    }else if(method == "SETUP"){
        onSetupRequest(socket_fd, cseq);
    }else if(method == "PLAY"){
        onPlayRequest(socket_fd, cseq);
    }
}

void RtspServer::handleMediaNotify(const sp<AMessage> &msg){
    int32_t type;
    CHECK(msg->findInt32("type", &type));
    switch (type) {
        case kWhatSPSPPSData:
            {
                sp<ABuffer> data;
                CHECK(msg->findBuffer("data", &data));
                sps_pps.clear();
                sps_pps.insert(sps_pps.end(), data->data(), data->data()+data->capacity());
                //sendSPSPPS((const unsigned char*)&sps_pps[0], sps_pps.size(), 0);
            }
            break;
        case kWhatVideoFrameData:
            {
                sp<ABuffer> data;
                CHECK(msg->findBuffer("data", &data));
                int64_t ptsUsec;
                CHECK(msg->findInt64("ptsUsec", &ptsUsec));
                sendVFrame(data->data(), data->capacity(), ptsUsec);
            }
            break;
        case kWhatAudioFrameData:
            break;
    }
}

void RtspServer::handleClientSocketExit(const sp<AMessage> &msg){
    FLOGE("onMessageReceived kWhatClientSocketExit!");
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
    FLOGE("work_sockets size=%d.", work_sockets.empty()?0:((int)work_sockets.size()));
    for(std::vector<int32_t>::iterator iter=work_sockets.begin();iter!=work_sockets.end();iter++){
        if(*iter==socket_fd){
            work_sockets.erase(iter);
            break;
        }
    }
    FLOGE("work_sockets size=%d.", work_sockets.empty()?0:((int)work_sockets.size()));
    if(work_sockets.empty()) mScreenDisplay->stopRecord();
}

status_t RtspServer::onOptionsRequest(int socket_fd, int32_t cseq) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n");
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onDescribeRequest(int socket_fd, int32_t cseq) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    AString spd;
    spd.append("v=0\r\n");
    spd.append("o=- 1627453750119587 1 in IP4 192.168.137.11\r\n");
    spd.append("t=0 0\r\n");
    spd.append("a=contol:*\r\n");
    spd.append("m=video 0 RTP/AVP/TCP 96\r\n");
    spd.append("a=rtpmap:96 H264/90000\r\n");
    //spd.append("a=fmtp:96 profile-level-id=420010;packetization-mode=1;sprop-parameter-sets=Z0KAFtoGQW/llIKDAwNoUJqA,aM4G4g==\r\n");
    spd.append("a=control:track1\r\n\r\n");
    char temp[128];
    sprintf(temp, "Content-Length: %d\r\n",(int)spd.size());
    response.append(temp);
    response.append("Content-Type: application/sdp\r\n");
    response.append("\r\n");
    response.append(spd.c_str());
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onSetupRequest(int socket_fd, int32_t cseq)
{
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
    char temp[128];
    sprintf(temp, "Session: %d\r\n",socket_fd);
    response.append(temp);
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onPlayRequest(int socket_fd, int32_t cseq)
{
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Range: npt=0.000-\r\n");
    char temp[128];
    sprintf(temp, "Session: %d\r\n",socket_fd);
    response.append(temp);
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    work_sockets.push_back(socket_fd);
    mScreenDisplay->stopRecord();
    mScreenDisplay->startRecord();
    return 0;
}

void RtspServer::sendSPSPPS(const unsigned char* sps_pps, int size, int64_t ptsUsec)
{
    char t_rtp_head[16];
    memset(t_rtp_head,0,16);
    t_rtp_head[0] = '$';
    t_rtp_head[1] = 0x01;
    t_rtp_head[2] = ((size+12) & 0xFF00 ) >> 8;
    t_rtp_head[3] = (size+12) & 0xFF;
    t_rtp_head[4] = 0x80;
    t_rtp_head[5] = 0x60;
    t_rtp_head[6] = (count & 0xFF00) >> 8;
    t_rtp_head[7] = count & 0xFF;
    t_rtp_head[8]  = (ptsUsec & 0xFF000000) >> 24;
    t_rtp_head[9]  = (ptsUsec & 0xFF0000) >> 16;
    t_rtp_head[10] = (ptsUsec & 0xFF00) >> 8;
    t_rtp_head[11] =  ptsUsec & 0xFF;
    count++;
    int vsize = work_sockets.size();
    for(int i=0;i<vsize;i++){
        int sendLen = send(work_sockets[i], t_rtp_head, 16, 0);
        sendLen = send(work_sockets[i], sps_pps, size, 0);
    }
}

void RtspServer::sendVFrame(const unsigned char* frame, int size, int64_t ptsUsec)
{
    unsigned char nalu = frame[0];
    if((nalu&0x1F)==5){
        sendSPSPPS((const unsigned char*)&sps_pps[0], sps_pps.size(), ptsUsec);
    }
    int fuanum = 0x1000-18;
    if(size <= fuanum){
        char t_rtp_head[16];
        memset(t_rtp_head,0,16);
        t_rtp_head[0]  = '$';
        t_rtp_head[1]  = 0x01;
        t_rtp_head[2]  = ((size+12) & 0xFF00 ) >> 8;
        t_rtp_head[3]  = (size+12) & 0xFF;
        t_rtp_head[4]  = 0x80;
        t_rtp_head[5]  = 0x60;
        t_rtp_head[6]  = (count & 0xFF00) >> 8;
        t_rtp_head[7]  = count & 0xFF;
        t_rtp_head[8]  = (ptsUsec & 0xFF000000) >> 24;
        t_rtp_head[9]  = (ptsUsec & 0xFF0000) >> 16;
        t_rtp_head[10] = (ptsUsec & 0xFF00) >> 8;
        t_rtp_head[11] =  ptsUsec & 0xFF;
        count++;
        int vsize = work_sockets.size();
        for(int i=0; i<vsize; i++){
            int sendLen = send(work_sockets[i],t_rtp_head,16,0);
            sendLen = send(work_sockets[i],frame,size,0);
        }
    } else {
        int num = 0;
        while((size-1-num*fuanum)>0){
            bool first = (num==0);
            bool last = ((size -1 - num * fuanum)<=fuanum);
            int rtpsize = last?(size -1 - num * fuanum) : fuanum;
            char t_rtp_head[18];
            memset(t_rtp_head,0,18);
            t_rtp_head[0]  = '$';
            t_rtp_head[1]  = 0x01;
            t_rtp_head[2]  = ((rtpsize+14) & 0xFF00 ) >> 8;
            t_rtp_head[3]  = (rtpsize+14) & 0xFF;
            t_rtp_head[4]  = 0x80;
            t_rtp_head[5]  = 0x60;
            t_rtp_head[6]  = (count & 0xFF00) >> 8;
            t_rtp_head[7]  = count & 0xFF;
            t_rtp_head[8]  = (ptsUsec & 0xFF000000) >> 24;
            t_rtp_head[9]  = (ptsUsec & 0xFF0000) >> 16;
            t_rtp_head[10] = (ptsUsec & 0xFF00) >> 8;
            t_rtp_head[11] =  ptsUsec & 0xFF;
            t_rtp_head[16] =  (nalu&0xE0)|0x1C;
            t_rtp_head[17] =  first?(0x80|(nalu&0x1F)):(last?(0x40|(nalu&0x1F)):(nalu&0x1F));
            count++;
            int vsize = work_sockets.size();
            for(int i=0; i<vsize; i++){
                int sendLen = send(work_sockets[i],t_rtp_head,18,0);
                sendLen = send(work_sockets[i],frame+num*fuanum+1,rtpsize,0);
            }
            //char temp[1024] = {0};
            //for (int j = 16; j < 18; j++) {
            //    sprintf(temp, "%s0x%02x,", temp, t_rtp_head[j]);
            //}
            //sprintf(temp, "%s0x%02x", temp, nalu);
            //FLOGE("SEND FU-A[%s][%d][%d][%d]", temp,rtpsize,num,size);
            num++;
        }
    }
}