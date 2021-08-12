//
// Created by FlyZebra on 2021/8/9 0009.
//

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>

#include "Controller.h"
#include "HandlerEvent.h"
#include "FlyLog.h"
#include "input.h"

using namespace android;

Controller::Controller()
{
}

Controller::~Controller()
{
}

void Controller::start()
{
    pthread_t init_socket_tid;
    int32_t ret = pthread_create(&init_socket_tid, nullptr, _controller_socket, (void *) this);
    if (ret != 0) {
    	FLOGE("create controller socket thread error!");
    	exit(-1);
    }
}

void *Controller::_controller_socket(void *argv)
{
	FLOGE("_controller_socket start!");
    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(9008);
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
		sp<AMessage> msg = new AMessage(kWhatClientSocket, (Controller *) argv);
		msg->setInt32("socket", client_socket);
		msg->post();
    }
    close(server_socket);
    server_socket = -1;
    FLOGE("_controller_socket exit!");
	return 0;
}

void *Controller::_controller_client_socket(void *argv){
    FLOGE("_controller_client_socket start!");
    signal(SIGPIPE, SIG_IGN);
    auto *p=(Controller *)argv;
    int32_t socket_fd;
    {
	    Mutex::Autolock autoLock(p->mLock);
	    socket_fd = p->thread_sockets.back();
	    p->thread_sockets.pop_back();
	}
	char recvBuf[1024];
    int32_t recvLen = -1;
    int32_t key_fd = open("/dev/input/event0",O_RDWR);
    int32_t touch_fd = open("/dev/input/event2",O_RDWR);
	while(!p->isStoped){
	    memset(recvBuf,0,1024);
	    recvLen = recv(socket_fd, recvBuf, 1024, 0);
	    FLOGD("sever_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, recvBuf);
        if (recvLen <= 0) {
            sp<AMessage> msg = new AMessage(kWhatClientSocketExit, (Controller *) argv);
            msg->setInt32("socket", socket_fd);
            msg->post();
            close(socket_fd);
            break;
        }else {
            char temp[4096] = {0};
            for (int32_t i = 0; i < recvLen; i++) {
                sprintf(temp, "%s%02x:", temp, recvBuf[i]);
            }
            FLOGE("keyevent_recv:len=[%d],errno=[%d]\n%s", recvLen, errno, temp);
            switch (recvBuf[0]){
                //HOME 键
                case 0x00:
                    p->input_key(key_fd, KEY_HOME);
                    break;
                //触摸屏幕
                case 0x02:
                    if(recvLen>=28 && recvBuf[27] == 1){
                        int action = recvBuf[1];
                        int32_t x = (recvBuf[10]<<24)+ (recvBuf[11]<<16) + (recvBuf[12]<<8) + recvBuf[13];
                        int32_t y = (recvBuf[14]<<24)+ (recvBuf[15]<<16) + (recvBuf[16]<<8) + recvBuf[17];
                        int32_t w = (recvBuf[18]<<8) + recvBuf[19];
                        int32_t h = (recvBuf[20]<<8) + recvBuf[21];
                        p->input_touch(touch_fd, x, y, action== 1 ? 0 : 1);
                    }
                    break;
                //中间滚动
                case 0x03:
                    if(recvLen>=21){
                        int32_t x = 0x21C;
                        int32_t y = 0x3C0;
                        if(recvBuf[20]==0x01){
                            for(int i=0;i<10;i++) {
                                p->input_touch(touch_fd, x, y + i * 10, 1);
                                usleep(1000);
                            }
                            p->input_touch(touch_fd, x, y + 100, 1);
                            p->input_touch(touch_fd, x, y + 100, 0);
                        }else{
                            for(int i=0;i<10;i++) {
                                p->input_touch(touch_fd, x, y - i * 10, 1);
                                usleep(1000);
                            }
                            p->input_touch(touch_fd, x, y - 100, 1);
                            p->input_touch(touch_fd, x, y - 100, 0);
                        }
                    }
                    break;
                case 0x04:
                    p->input_key(key_fd, KEY_BACK);
                    break;
            }
        }
	}
	close(key_fd);
	close(touch_fd);
	FLOGE("_controller_client_socket exit!");
	return 0;
}

void Controller::onMessageReceived(const sp<AMessage> &msg){
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

void Controller::handleClientSocket(const sp<AMessage> &msg)
{
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
    Mutex::Autolock autoLock(mLock);
    thread_sockets.push_back(socket_fd);
    pthread_t client_socket_tid;
    int32_t ret = pthread_create(&client_socket_tid, nullptr, _controller_client_socket, (void *)this);
    if (ret != 0) {
    	FLOGE("create client socket thread error!");
    	thread_sockets.pop_back();
    }
}

void Controller::handleSocketRecvData(const sp<AMessage> &msg)
{
    int32_t socket_fd;
    CHECK(msg->findInt32("socket", &socket_fd));
	sp<ABuffer> data;
    CHECK(msg->findBuffer("data", &data));
}

void Controller::handleClientSocketExit(const sp<AMessage> &msg)
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

void Controller::input_key(int32_t fd, int32_t key)
{
    input_event _event1;
    memset(&_event1,0,sizeof(_event1));
    _event1.type = EV_KEY;
    _event1.code = key;
    _event1.value = 1;
    int32_t ret = write(fd,&_event1,sizeof(_event1));
    input_event _event2;
    memset(&_event2,0,sizeof(_event2));
    _event2.type = EV_SYN;
    _event2.code = SYN_REPORT;
    _event2.value = 0;
    ret = write(fd,&_event2,sizeof(_event2));
    input_event _event3;
    memset(&_event3,0,sizeof(_event3));
    _event3.type = EV_KEY;
    _event3.code = key;
    _event3.value = 0;
    ret = write(fd,&_event3,sizeof(_event3));
    input_event _event4;
    memset(&_event4,0,sizeof(_event4));
    _event4.type = EV_SYN;
    _event4.code = SYN_REPORT;
    _event4.value = 0;
    ret = write(fd,&_event4,sizeof(_event4));
}

void Controller::input_touch(int32_t fd, int32_t x, int32_t y, int32_t action)
{
    input_event _event1;
    memset(&_event1,0,sizeof(_event1));
    _event1.type = EV_ABS;
    _event1.code = ABS_MT_POSITION_X;
    _event1.value = x;
    int32_t ret = write(fd,&_event1,sizeof(_event1));
    input_event _event2;
    memset(&_event2,0,sizeof(_event2));
    _event2.type = EV_ABS;
    _event2.code = ABS_MT_POSITION_Y;
    _event2.value = y;
    ret = write(fd,&_event2,sizeof(_event2));
    input_event _event3;
    memset(&_event3,0,sizeof(_event3));
    _event3.type = EV_ABS;
    _event3.code = ABS_MT_TRACKING_ID;
    _event3.value = 0;
    ret = write(fd,&_event3,sizeof(_event3));
    input_event _event4;
    memset(&_event4,0,sizeof(_event4));
    _event4.type = EV_SYN;
    _event4.code = SYN_MT_REPORT;
    _event4.value = 0;
    ret = write(fd,&_event4,sizeof(_event4));
    input_event _event5;
    memset(&_event5,0,sizeof(_event5));
    _event5.type = EV_KEY;
    _event5.code = BTN_TOUCH;
    _event5.value = action;
    ret = write(fd,&_event5,sizeof(_event5));
    input_event _event6;
    memset(&_event6,0,sizeof(_event6));
    _event6.type = EV_SYN;
    _event6.code = SYN_REPORT;
    _event6.value = 0;
    ret = write(fd,&_event6,sizeof(_event6));
}
