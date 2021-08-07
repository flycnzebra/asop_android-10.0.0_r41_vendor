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
#include <error.h>
#include <unistd.h>
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
    char temp[16];
    sprintf(temp, "CSeq: %d\r\n",cseq);
    response->append(temp);
    response->append("User-Agent: Android screen rtsp(author zebra)\r\n");
    time_t now = time(NULL);
    struct tm *now2 = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);
    response->append("Date: ");
    response->append(buf);
    response->append("\r\n");
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

void *RtspServer::_server_socket(void *argv){
	FLOGE("_server_socket start!");
	//Bind TCP SOCKET
    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(554);

    int32_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        FLOGE("socket error %s errno: %d", strerror(errno), errno);
        return 0;
    }

    int32_t ret = bind(server_socket,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        FLOGE( "bind socket error %s errno: %d", strerror(errno), errno);
        return 0;
    }
    ret = listen(server_socket, 5);
    if (ret < 0) {
        FLOGE("listen error %s errno: %d", strerror(errno), errno);
    }
    for(;;) {
        int32_t client_socket = accept(server_socket, (struct sockaddr*)NULL, NULL);
        if(client_socket < 0) {
            FLOGE("accpet socket error: %s errno :%d", strerror(errno), errno);
            continue;
        }
		sp<AMessage> msg = new AMessage(kWhatClientSocket, (RtspServer *) argv);
		msg->setInt32("socket", client_socket);
		msg->post();
    }
    close(server_socket);
    server_socket = -1;
    FLOGE("_server_socket exit!");
	return 0;
}

void *RtspServer::_client_socket(void *argv){
    FLOGE("_client_socket start!");
    signal(SIGPIPE, SIG_IGN);
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
	while(!p->isStoped){
	    memset(recvBuf,0,1024);
	    recvLen = recv(socket_fd, recvBuf, 1024, 0);
	    FLOGE("tcp_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
        if (recvLen <= 0) {
            sp<AMessage> msg = new AMessage(kWhatClientSocketExit, (RtspServer *) argv);
            msg->setInt32("socket", socket_fd);
            msg->post();
            close(socket_fd);
            break;
        }else {
            sp<ABuffer> data = ABuffer::CreateAsCopy(recvBuf, recvLen);
            sp<AMessage> msg = new AMessage(kWhatSocketRecvData, (RtspServer *) argv);
            msg->setBuffer("data", data);
            msg->setInt32("socket", socket_fd);
            msg->post();
        }
	}
	FLOGE("_client_socket exit!");
	return 0;
}

void *RtspServer::_rtpudp_socket(void *argv){
    FLOGE("_rtpudp_socket start!");
    signal(SIGPIPE, SIG_IGN);
    auto *p=(RtspServer *)argv;
    int32_t socket_fd = p->rtp_socket;
    char recvBuf[1024];
    int32_t addr_len;
    int32_t recvLen = -1;
    struct sockaddr_in addr_client;
    while(!p->isStoped){
        memset(recvBuf,0, 1024);
        int32_t recvLen = recvfrom(socket_fd, recvBuf, 1024, 0, (struct sockaddr *)&addr_client, (socklen_t *)&addr_len);
        FLOGE("rtp_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
    }
    FLOGE("_rtpudp_socket exit!");
    return 0;
}

void *RtspServer::_rtcpudp_socket(void *argv){
    FLOGE("_rtcpudp_socket start!");
    signal(SIGPIPE, SIG_IGN);
    auto *p=(RtspServer *)argv;
    int32_t socket_fd = p->rtcp_socket;
    char recvBuf[1024];
    int32_t addr_len;
    int32_t recvLen = -1;
    struct sockaddr_in addr_client;
    while(!p->isStoped){
        memset(recvBuf,0, 1024);
        int32_t recvLen = recvfrom(socket_fd, recvBuf, 1024, 0, (struct sockaddr *)&addr_client, (socklen_t *)&addr_len);
        FLOGE("rtcp_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
    }
    FLOGE("_rtcpudp_socket exit!");
    return 0;
}

void RtspServer::handleStart(const sp<AMessage> &msg)
{
    //Bind UDP 1 SOCKET
    rtp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(rtp_socket < 0) {
       FLOGE("RTP udp socket error %s errno: %d", strerror(errno), errno);
       exit(-1);
    }
    struct sockaddr_in addr_serv1;
    memset(&addr_serv1, 0, sizeof(struct sockaddr_in));//每个字节都用0填充
    addr_serv1.sin_family = AF_INET;//使用IPV4地址
    addr_serv1.sin_port = htons(9002);//端口
    addr_serv1.sin_addr.s_addr = htonl(INADDR_ANY);//自动获取IP地址
    int32_t ret = bind(rtp_socket, (struct sockaddr *)&addr_serv1, sizeof(addr_serv1));
    if(ret < 0){
      FLOGE( "bind RTP udp socket error %s errno: %d", strerror(errno), errno);
      exit(-1);
    }
    pthread_t rtpudp_socket_tid;
    ret = pthread_create(&rtpudp_socket_tid, nullptr, _rtpudp_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create RTP udp socket thread error!");
    	exit(-1);
    }

    //Bind UDP 2 SOCKET
    rtcp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(rtcp_socket < 0) {
       FLOGE("RTCP udp socket error %s errno: %d", strerror(errno), errno);
       exit(-1);
    }
    struct sockaddr_in addr_serv2;
    memset(&addr_serv2, 0, sizeof(struct sockaddr_in));//每个字节都用0填充
    addr_serv2.sin_family = AF_INET;//使用IPV4地址
    addr_serv2.sin_port = htons(9003);//端口
    addr_serv2.sin_addr.s_addr = htonl(INADDR_ANY);//自动获取IP地址
    ret = bind(rtcp_socket, (struct sockaddr *)&addr_serv2, sizeof(addr_serv2));
    if(ret < 0){
      FLOGE( "bind RTCP udp socket error %s errno: %d", strerror(errno), errno);
      exit(-1);
    }
    pthread_t rtcpudp_socket_tid;
    ret = pthread_create(&rtcpudp_socket_tid, nullptr, _rtcpudp_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create RTCP udp socket thread error!");
    	exit(-1);
    }

    //TCP 554 socket
    pthread_t init_socket_tid;
    ret = pthread_create(&init_socket_tid, nullptr, _server_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create rtsp socket thread error!");
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
    int32_t ret = pthread_create(&client_socket_tid, nullptr, _client_socket, (void *)this);
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
	sp<ABuffer> data;
    CHECK(msg->findBuffer("data", &data));
    char action[64] = {0};
    char url[512] = {0};
    char ver[64] = {0};
    if(sscanf((const char*)data->data(), "%s %s %s\r\n", action, url, ver) == 3) {
        int32_t cseq;
    	sscanf(strstr((const char*)data->data(), "CSeq"), "CSeq: %d", &cseq);
        std::string method(action);
        if (method == "OPTIONS") {
            onOptionsRequest((const char*)data->data(),socket_fd, cseq);
        }else if(method == "DESCRIBE"){
            onDescribeRequest((const char*)data->data(),socket_fd, cseq);
        }else if(method == "SETUP"){
            onSetupRequest((const char*)data->data(),socket_fd, cseq);
        }else if(method == "PLAY"){
            onPlayRequest((const char*)data->data(),socket_fd, cseq);
        }else if(method == "GET_PARAMETER"){
            onGetParameterRequest((const char*)data->data(),socket_fd, cseq);
        }
    }else{
        onOtherRequest((const char*)data->data(),socket_fd, -1);
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
    int32_t size = work_sockets.empty()?0:((int)work_sockets.size());
    for(int32_t i=0;i<size;i++){
        if(work_sockets[i].socket == socket_fd){
            work_sockets.erase(work_sockets.begin()+i);
            break;
        }
    }
    FLOGE("work_sockets size=%d.", work_sockets.empty()?0:((int)work_sockets.size()));
    if(work_sockets.empty()) mScreenDisplay->stopRecord();
}

status_t RtspServer::onOptionsRequest(const char* data, int32_t socket_fd, int32_t cseq) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n");
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onDescribeRequest(const char* data, int32_t socket_fd, int32_t cseq) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    AString spd;
    spd.append("v=0\r\n");
    spd.append("o=- 1627453750119587 1 in IP4 192.168.137.11\r\n");
    spd.append("t=0 0\r\n");
    spd.append("a=contol:*\r\n");
    spd.append("m=video 0 RTP/AVP 96\r\n");
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

status_t RtspServer::onSetupRequest(const char* data, int32_t socket_fd, int32_t cseq)
{
    FLOGE("onSetupRequest.");
    struct client_connect conn;
    conn.socket = socket_fd;
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    char field[16];
    if (strncmp(strstr((const char*)data, "RTP/AVP"), "RTP/AVP/UDP", 11) == 0) {
        conn.type = RTP_UDP;
        const char *temp = strstr((const char*)data, "client_port=");
        if(temp!=nullptr){
            sscanf(temp, "client_port=%d-%d", &conn.rtp_port, &conn.rtcp_port);
        }
        getpeername(socket_fd, (struct sockaddr *)&conn.addr_in, (socklen_t*)&conn.addrLen);
        conn.addr_in.sin_port = htons(conn.rtp_port);
        response.append("Transport: RTP/AVP/UDP;unicast;server_port=9002-9003;interleaved=0-1\r\n");
    }else{
        conn.type = RTP_TCP;
        response.append("Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
    }
    char temp[128];
    sprintf(temp, "Session: %d\r\n",socket_fd);
    response.append(temp);
    response.append("\r\n");
    conn.status = S_SETUP;
    work_sockets.push_back(conn);
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onPlayRequest(const char* data, int32_t socket_fd, int32_t cseq)
{
    int32_t size = work_sockets.empty()?0:((int)work_sockets.size());
    for(int32_t i=0;i<size;i++){
        if(work_sockets[i].socket == socket_fd){
            work_sockets[i].status = S_PLAY;
            break;
        }
    }
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Range: npt=0.000-\r\n");
    char temp[128];
    sprintf(temp, "Session: %d\r\n",socket_fd);
    response.append(temp);
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    FLOGE("work_sockets size=%d.", work_sockets.empty()?0:((int)work_sockets.size()));
    mScreenDisplay->stopRecord();
    mScreenDisplay->startRecord();
    return 0;
}

status_t RtspServer::onGetParameterRequest(const char* data, int32_t socket_fd, int32_t cseq)
{
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

status_t RtspServer::onOtherRequest(const char* data, int32_t socket_fd, int32_t cseq)
{
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("\r\n");
    send(socket_fd,response.c_str(),response.size(),0);
    FLOGE("send:len=[%d],errno=[%d]\n%s",(int)response.size(), errno, response.c_str());
    return 0;
}

void RtspServer::sendSPSPPS(const unsigned char* sps_pps, int32_t size, int64_t ptsUsec)
{
    serialnumber++;
    char rtp_pack[16 + size];
    rtp_pack[0] = '$';
    rtp_pack[1] = 0x01;
    rtp_pack[2] = ((size+12) & 0xFF00 ) >> 8;
    rtp_pack[3] = (size+12) & 0xFF;
    rtp_pack[4] = 0x80;
    rtp_pack[5] = 0x60;
    rtp_pack[6] = (serialnumber & 0xFF00) >> 8;
    rtp_pack[7] = serialnumber & 0xFF;
    rtp_pack[8]  = (ptsUsec & 0xFF000000) >> 24;
    rtp_pack[9]  = (ptsUsec & 0xFF0000) >> 16;
    rtp_pack[10] = (ptsUsec & 0xFF00) >> 8;
    rtp_pack[11] =  ptsUsec & 0xFF;
    memcpy(rtp_pack+16, sps_pps, size);
    int32_t vsize = work_sockets.size();
    int32_t sendLen;
    for(int32_t i=0;i<vsize;i++){
        if(work_sockets[i].type==RTP_TCP){
            sendLen = send(work_sockets[i].socket, rtp_pack, 16+size, 0);
        }else{
            struct sockaddr_in peer_addr_;
            socklen_t addrLen;
            getpeername(work_sockets[i].socket, (struct sockaddr *)&peer_addr_, (socklen_t*)&addrLen);
            peer_addr_.sin_port = htons(work_sockets[i].rtp_port);
            sendto(rtp_socket, rtp_pack+4, 12+size, 0, (struct sockaddr*)&peer_addr_, addrLen);
        }
    }
}

void RtspServer::sendVFrame(const unsigned char* frame, int32_t size, int64_t ptsUsec)
{
    unsigned char nalu = frame[0];
    if((nalu&0x1F)==5){
        sendSPSPPS((const unsigned char*)&sps_pps[0], sps_pps.size(), ptsUsec);
    }
    int32_t fau_num = 1280 - 18;
    if(size <= fau_num){
        serialnumber++;
        char rtp_pack[16 + size];
        rtp_pack[0]  = '$';
        rtp_pack[1]  = 0x01;
        rtp_pack[2]  = ((size+12) & 0xFF00 ) >> 8;
        rtp_pack[3]  = (size+12) & 0xFF;
        rtp_pack[4]  = 0x80;
        rtp_pack[5]  = 0x60;
        rtp_pack[6]  = (serialnumber & 0xFF00) >> 8;
        rtp_pack[7]  = serialnumber & 0xFF;
        rtp_pack[8]  = (ptsUsec & 0xFF000000) >> 24;
        rtp_pack[9]  = (ptsUsec & 0xFF0000) >> 16;
        rtp_pack[10] = (ptsUsec & 0xFF00) >> 8;
        rtp_pack[11] =  ptsUsec & 0xFF;
        memcpy(rtp_pack+16, frame, size);
        int32_t vsize = work_sockets.size();
        int32_t sendLen = -1;
        for(int32_t i=0; i<vsize; i++){
            if(work_sockets[i].type==RTP_TCP){
                sendLen = send(work_sockets[i].socket,rtp_pack,16+size,0);
            }else{
                struct sockaddr_in peer_addr_;
                socklen_t addrLen;
                getpeername(work_sockets[i].socket, (struct sockaddr *)&peer_addr_, (socklen_t*)&addrLen);
                peer_addr_.sin_port = htons(work_sockets[i].rtp_port);
                sendto(rtp_socket, rtp_pack+4, 12+size, 0, (struct sockaddr*)&peer_addr_,addrLen);
            }
        }
    } else {
        int32_t num = 0;
        while((size-1-num*fau_num)>0){
            bool first = (num==0);
            bool last = ((size -1 - num * fau_num)<=fau_num);
            int32_t rtpsize = last?(size -1 - num * fau_num) : fau_num;
            serialnumber++;
            char rtp_pack[18 + rtpsize];
            rtp_pack[0]  = '$';
            rtp_pack[1]  = 0x01;
            rtp_pack[2]  = ((rtpsize+14) & 0xFF00 ) >> 8;
            rtp_pack[3]  = (rtpsize+14) & 0xFF;
            rtp_pack[4]  = 0x80;
            rtp_pack[5]  = 0x60;
            rtp_pack[6]  = (serialnumber & 0xFF00) >> 8;
            rtp_pack[7]  = serialnumber & 0xFF;
            rtp_pack[8]  = (ptsUsec & 0xFF000000) >> 24;
            rtp_pack[9]  = (ptsUsec & 0xFF0000) >> 16;
            rtp_pack[10] = (ptsUsec & 0xFF00) >> 8;
            rtp_pack[11] =  ptsUsec & 0xFF;
            rtp_pack[16] =  (nalu&0xE0)|0x1C;
            rtp_pack[17] =  first?(0x80|(nalu&0x1F)):(last?(0x40|(nalu&0x1F)):(nalu&0x1F));
            memcpy(rtp_pack+18, frame+num*fau_num+1, rtpsize);
            int32_t vsize = work_sockets.size();
            for(int32_t i=0; i<vsize; i++){
                if(work_sockets[i].type==RTP_TCP){
                    int32_t sendLen = send(work_sockets[i].socket,rtp_pack,18+rtpsize,0);
                }else{
                    struct sockaddr_in peer_addr_;
                    socklen_t addrLen;
                    getpeername(work_sockets[i].socket, (struct sockaddr *)&peer_addr_, (socklen_t*)&addrLen);
                    peer_addr_.sin_port = htons(work_sockets[i].rtp_port);
                    sendto(rtp_socket, rtp_pack+4, 14+rtpsize, 0, (struct sockaddr*)&peer_addr_, addrLen);
                }
            }
            //char temp[1024] = {0};
            //for (int32_t j = 16; j < 18; j++) {
            //    sprintf(temp, "%s0x%02x,", temp, rtp_pack[j]);
            //}
            //sprintf(temp, "%s0x%02x", temp, nalu);
            //FLOGE("SEND FU-A[%s][%d][%d][%d]", temp,rtpsize,num,size);
            num++;
        }
    }
}