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
#include <regex.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include "RtspServer.h"
#include "screenrecord.h"
#include "FlyLog.h"

using namespace android;

RtspServer::RtspServer()
{
}

void RtspServer::network(){
    char buff[1024];
    int recv_len = -1;
    int conn_fd = -1;
    int ret = -1;
    int server_ip_port = 554;

    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(server_ip_port);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
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
         memset(buff,0,1024);
        recv_len = recv(conn_fd, buff, 1024, 0);
        if (recv_len < 0) {
            FLOGE("recv error %s errno: %d", strerror(errno), errno);
            continue;
        }
        FLOGE("recv:\n %s", buff);

        std::string option = "RTSP/1.0 200 OK\r\n";
        option.append("Server: FlyZEBRA V1.0\r\n");
        option.append("CSeq: 1\r\n");
        option.append("Public: www.flyzebra.screen, GET_PARAMETER, SET_PARAMETER\r\n");
        option.append("\r\n");
        send(conn_fd,option.c_str(),option.size(),0);

        memset(buff,0,1024);
        recv_len = recv(conn_fd, buff, 1024, 0);
        if (recv_len < 0) {
            FLOGE("recv error %s errno: %d", strerror(errno), errno);
            continue;
        }
        FLOGE("recv:\n %s", buff);

        std::string describe = "RTSP/1.0 200 OK\r\n";
        option.append("Server: FlyZEBRA V1.0\r\n");
        option.append("CSeq: 2\r\n");
        option.append("Public: www.flyzebra.screen, GET_PARAMETER, SET_PARAMETER\r\n");
        option.append("\r\n");
        send(conn_fd,option.c_str(),option.size(),0);

        close(conn_fd);
        conn_fd = -1;
    }

    close(listen_fd);
    listen_fd = -1;
}

void RtspServer::onMessageReceived(const sp<AMessage> &msg){
    switch (msg->what()) {
        case kWhatStart:
            sp<AReplyToken> replyID;
            msg->senderAwaitsResponse(&replyID);
            status_t err = OK;
            sp<AMessage> response = new AMessage;
            response->setInt32("err", err);
            response->postReply(replyID);
            FLOGE("onMessageReceived kWhatStart");
            break;
    }
}

status_t RtspServer::start(){
    FLOGE("RtspServer start");
    sp<AMessage> msg = new AMessage(kWhatStart, this);
    sp<AMessage> response;
    status_t err = msg->postAndAwaitResponse(&response);
    if (err != OK) {
        return err;
    }
    if (response == NULL || !(response)->findInt32("err", &err)) {
        err = OK;
    }
    return err;
}

RtspServer::~RtspServer()
{
}

void RtspServer::sendVideoFrame()
{
}


