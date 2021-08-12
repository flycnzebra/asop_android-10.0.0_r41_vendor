//
// Created by FlyZebra on 2021/8/12 0012.
//
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>

#include "AudioStream.h"
#include "HandlerEvent.h"
#include "FlyLog.h"

using namespace android;

AudioStream::AudioStream()
{
}

AudioStream::~AudioStream()
{
}

void AudioStream::start()
{
    pthread_t init_socket_tid;
    int32_t ret = pthread_create(&init_socket_tid, nullptr, _audio_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create audio socket thread error!");
    	exit(-1);
    }
}

void AudioStream::onMessageReceived(const sp<AMessage> &msg)
{
    switch (msg->what()) {
       case kWhatClientSocket:
           handleClientSocket(msg);
           break;
       case kWhatClientSocketExit:
           handleClientSocketExit(msg);
           break;
       case kWhatSocketRecvData:
           handleSocketRecvData(msg);
           break;
    }
}

void *AudioStream::_audio_socket(void *argv)
{
	FLOGE("_audio_socket start!");
    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(9007);
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
		sp<AMessage> msg = new AMessage(kWhatClientSocket, (AudioStream *) argv);
		msg->setInt32("socket", client_socket);
		msg->post();
    }
    close(server_socket);
    server_socket = -1;
    FLOGE("_audio_socket exit!");
	return 0;
}

void *AudioStream::_audio_client_socket(void *argv)
{
    FLOGE("_audio_client_socket start!");
    signal(SIGPIPE, SIG_IGN);
    auto *p=(AudioStream *)argv;
    int32_t socket_fd;
    {
	    Mutex::Autolock autoLock(p->mLock);
	    socket_fd = p->thread_sockets.back();
	    p->thread_sockets.pop_back();
	}
	char recvBuf[1024];
    int32_t recvLen = -1;
	while(!p->isStoped){
	    memset(recvBuf,0,1024);
	    recvLen = recv(socket_fd, recvBuf, 1024, 0);
	    FLOGD("sever_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
	}
	FLOGE("_audio_client_socket exit!");
	return 0;
}

void AudioStream::handleClientSocket(const sp<AMessage> &msg)
{
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
    Mutex::Autolock autoLock(mLock);
    thread_sockets.push_back(socket_fd);
    pthread_t client_socket_tid;
    int32_t ret = pthread_create(&client_socket_tid, nullptr, _audio_client_socket, (void *)this);
    if (ret != 0) {
    	FLOGE("create client socket thread error!");
    	thread_sockets.pop_back();
    }
}

void AudioStream::handleSocketRecvData(const sp<AMessage> &msg)
{
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
	sp<ABuffer> data;
    CHECK(msg->findBuffer("data", &data));
}

void AudioStream::handleClientSocketExit(const sp<AMessage> &msg)
{
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
    int32_t size = conn_sockets.empty()?0:((int)conn_sockets.size());
    for(int32_t i=0; i<size; i++){
        if(conn_sockets[i].socket == socket_fd){
            conn_sockets.erase(conn_sockets.begin()+i);
            break;
        }
    }
    FLOGD("conn_sockets size=%d.", conn_sockets.empty()?0:((int)conn_sockets.size()));
}