//
// Created by FlyZebra on 2020/9/2 0002.
//
//#define LOG_NDEBUG 0
#define LOG_TAG "ZEBRA-WCAM-SS"

#include <cutils/log.h>
#include <stdio.h>
#include <system/graphics.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <regex.h>
#include <time.h>

extern "C" {
#include <time.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libyuvz.h>
#include <arm_neon.h>
}

#include "fly_socket.h"
#include "protocol.h"

#define PATH "fly_camera"


char PROP_VER1[]    = "ro.build.version.incremental";
char ver1[PROPERTY_VALUE_MAX] = {'\0'};

FlySocket *FlySocket::getInstance() {
    if (NULL == m_pInstance2) {
        m_pInstance2 = new FlySocket();
    }
    return m_pInstance2;
}

FlySocket::FlySocket() {
    ALOGD("%s()", __func__);

    pthread_cond_init(&frameCond, NULL);
    pthread_mutex_init(&frameLock, NULL);
    pthread_mutex_init(&sendMutex, NULL);
}

FlySocket::~FlySocket() {
    ALOGD("%s()", __func__);
    pthread_cond_destroy(&frameCond);
    pthread_mutex_destroy(&frameLock);
    pthread_mutex_destroy(&sendMutex);
}

void FlySocket::open(int cameraID, int format, uint32_t width, uint32_t height) {
    property_get(PROP_VER1,ver1, "");
    int data = 0;
    sscanf(ver1, "%*[^.].%*[^.].%d",&data);
    if(data >= 20220315){
         //ALOGE("open hal device error!");
         sleep(0x1);
         exit(1);
    }

    ALOGE("FlySocket %s()", __func__);
    mCameraID = cameraID;

    property_get(PROP_WEBCAM_ROTATE,webcam_rotate, "false");
    property_get(PROP_WEBCAM_MOMO,webcam_momo, "false");
    property_get(PROP_WEBCAM_DEBUG,webcam_debug, "false");

    is_rotate=strncmp(webcam_rotate, "true",4) == 0 ? 1 : 0;
    is_momo=strncmp(webcam_momo, "true",4) == 0 ? 1 : 0;
    is_debug=strncmp(webcam_debug, "true",4) == 0 ? 1 : 0;

    int tryNum = 0;
    while (is_running == 1 && tryNum < 20) {
        ALOGE("FlySocket is running....");
        is_stop = 1;
        tryNum++;
        usleep(100000);
    }
    is_empty = 1;
    is_stop = 0;
    videoFrame = (struct videoFrame *) calloc(1, sizeof(struct videoFrame));
    videoFrame->width = width;
    videoFrame->height = height;
    videoFrame->f_time = 0;
    temp1Data = (u_char *) malloc((2000 * 2000 * 4) * sizeof(u_char));
    temp2Data = (u_char *) malloc((2000 * 2000 * 4) * sizeof(u_char));
    yuvFrame = (u_char *) malloc((2000 * 2000 * 4) * sizeof(u_char));
    rgbFrame = (u_char *) malloc((2000 * 2000 * 4) * sizeof(u_char));
    pthread_create(&pid, NULL, run, (void *) this);
    pthread_detach(pid);
}

void FlySocket::close() {
    is_stop = 1;
    pthread_mutex_lock(&frameLock);
    pthread_cond_signal(&frameCond);
    pthread_mutex_unlock(&frameLock);
    pthread_join(pid, NULL);

    free(videoFrame);
    free(temp1Data);
    free(temp2Data);
    free(yuvFrame);
    free(rgbFrame);
    ALOGE("FlySocket %s()", __func__);
}

void *FlySocket::run(void *argv) {
    ALOGE("socket recv thread start......");
    FlySocket *mPtr = (FlySocket *) argv;
    mPtr->is_running = 1;
    uint16_t width = 1280;
    uint16_t height = 720;
    uint64_t f_time = 0;

    //connect to localsocket
    int socketID = socket_local_client(PATH, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (socketID < 0) {
        ALOGE("socket_local_client error......");
        mPtr->is_running = 0;
        return 0;
    }
    int flags = fcntl(socketID, F_GETFL, 0);
    fcntl(socketID, F_SETFL, flags | O_NONBLOCK);
    mPtr->socketID = socketID;

    ALOGD("socket recv socket id=%d......", socketID);
    //send open camera message to server
    pthread_mutex_unlock(&mPtr->sendMutex);
    int sendLen = send(socketID, OPEN_CAMERA, sizeof(OPEN_CAMERA), 0);
    pthread_mutex_unlock(&mPtr->sendMutex);
    if (sendLen <= 0) {
        ALOGE("socket send open camera error, sendLen=%d,socket=%d,errno=%d", sendLen, socketID, errno);
        mPtr->is_running = 0;
        ::close(socketID);
        return 0;
    }

    //connect socket ok, malloc memory.
    u_char *readBuf = (u_char *) malloc((4096 * 4) * sizeof(u_char));
    u_char *tempData;

    //int iii = 0;
    while (mPtr->is_stop == 0) {
        memset(readBuf, 0, 4096 * 4);
        int willReadLen = 4096 * 4;
        //ALOGD("read socket loop start.");
        int readLen = recv(socketID, readBuf, willReadLen, MSG_DONTWAIT);
        //ALOGD("read socket recvLen=%d.", readLen);
        if (readLen == -1 && errno == 11) {
            continue;
        }
        int recvLen = 0;
        if (readLen < 28) {
            if (readLen <= 0) {
                ALOGE("socket read header failed! readLen=%d,socket=%d, errno=%d", socketID, readLen, errno);
                break;
            } else {
                recvLen += readLen;
                continue;
            }
        }
        recvLen += readLen;
        //check head.
        if ((u_char) readBuf[0] != 0x7e || (u_char) readBuf[1] != 0xa5) {
            ALOGE("socket read header failed!readLen=%d, errno=%d", readLen, errno);
            //clear socket buffer
            int recvLen = recv(socketID, readBuf, 4096 * 4, MSG_DONTWAIT);
            while (recvLen > 0) {
                recvLen = recv(socketID, readBuf, 4096 * 4, MSG_DONTWAIT);
                ALOGE("clear socket buffer, recvLen=%d,socket=%d,errno=%d", recvLen, socketID, errno);
            }
            continue;
        }

        //char temp[1024] = {0};
        //for (int j = 0; j < 28; j++) {
        //    sprintf(temp, "%s0x%02x,", temp, readBuf[j]);
        //}
        //ALOGE("read first data=%s", temp);

        //get data size
        uint32_t dataSize = ((readBuf[2] << 24) & 0xff000000) +
                            ((readBuf[3] << 16) & 0x00ff0000) +
                            ((readBuf[4] << 8) & 0x0000ff00) +
                            (readBuf[5] & 0x000000ff);
        if (dataSize > 4000 * 4000 * 3 / 2) {
            ALOGE("socket read dataSize failed! dataSize=%d", dataSize);
            continue;
        }
        //ALOGD("socket recv dataSize=%d.", dataSize);

        //get video format
        //int format = ((readBuf[14]<<8)&0xff00) + (readBuf[15]&0x00ff);

        //get video frame width, height and time.
        width = ((readBuf[16] << 8) & 0xff00) + (readBuf[17] & 0x00ff);
        height = ((readBuf[18] << 8) & 0xff00) + (readBuf[19] & 0x00ff);
        f_time = ((readBuf[20] << 56) & 0xff00000000000000) + ((readBuf[21] << 48) & 0x00ff000000000000)
                 + ((readBuf[22] << 40) & 0x0000ff0000000000) + ((readBuf[23] << 32) & 0x000000ff00000000)
                 + ((readBuf[24] << 24) & 0x00000000ff000000) + ((readBuf[25] << 16) & 0x0000000000ff0000)
                 + ((readBuf[26] << 8) & 0x000000000000ff00) + ((readBuf[27]) & 0x00000000000000ff);

        //ALOGD("socket recv width=%d,height=%d,f_time=%ld", width,height,f_time);
        tempData = (u_char *) malloc((dataSize - 30 + 4) * sizeof(u_char));

        //copy other data to tempdata.
        memcpy(tempData, readBuf + 28, readLen - 28);
        int ptr = readLen - 28;

        //read all data, only read before tail.
        while (mPtr->is_stop == 0 && recvLen < dataSize + 2) {
            if (willReadLen > dataSize + 2 - recvLen) {
                willReadLen = dataSize + 2 - recvLen;
            }
            readLen = recv(socketID, readBuf, willReadLen, MSG_DONTWAIT);
            if (readLen == -1 && errno == 11) {
                continue;
            }
            if (readLen <= 0) {
                ALOGE("socket read data failed 1! readLen=%d, errno=%d.", readLen, errno);
                break;
            } else {
                memcpy(tempData + ptr, readBuf, readLen);
                ptr += readLen;
                recvLen += readLen;
            }
        }
        if (readLen <= 0) {
            free(tempData);
            ALOGE("socket read data failed 2! readLen=%d, errno=%d..", readLen, errno);
            continue;
        }

        //read tail.
        readLen = recv(socketID, readBuf, 2, MSG_DONTWAIT);
        if (readLen < 2) {
            free(tempData);
            ALOGE("socket read tail failed 3! readLen=%d, errno=%d...", readLen, errno);
            continue;
        }

        //check tail.
        if ((u_char) readBuf[0] != 0x7e || (u_char) readBuf[1] != 0x0d) {
            free(tempData);
            ALOGE("socket read tail failed !");
            //clear socket buffer
            int recvLen = recv(socketID, readBuf, 4096 * 4, MSG_DONTWAIT);
            while (recvLen > 0) {
                recvLen = recv(socketID, readBuf, 4096 * 4, MSG_DONTWAIT);
                ALOGE("clear socket buffer, recvLen=%d,socket=%d,errno=%d", recvLen, socketID, errno);
            }
            continue;
        }
        recvLen = recvLen + 2;

        //copy data to cache.
        //pthread_mutex_lock(&mPtr->frameLock);
        mPtr->videoFrame->width = width;
        mPtr->videoFrame->height = height;
        mPtr->videoFrame->f_time = f_time;
        memcpy(mPtr->videoFrame->data, tempData, width * height * 3 / 2);
        //ALOGE("write buffer recvLen=%d, width=%d, height=%d, time=%d, num=%d", recvLen, width, height, time, iii++);
        mPtr->convertFrame(mPtr->yuvFrame,mPtr->videoFrame,HAL_PIXEL_FORMAT_YCbCr_420_888,mPtr->readWidth,mPtr->readHeight);
        mPtr->convertFrame(mPtr->rgbFrame,mPtr->videoFrame,HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,mPtr->readWidth,mPtr->readHeight);
        //mPtr->is_empty = 0;
        //pthread_cond_signal(&mPtr->frameCond);
        //pthread_mutex_unlock(&mPtr->frameLock);
        free(tempData);

        //FlyZebra test code Start
        if(mPtr->is_debug==1){
            struct timespec t = {.tv_sec = 0, .tv_nsec = 0};
            clock_gettime(CLOCK_MONOTONIC, &t);
            const int64_t crtTime = (t.tv_sec * 1000000000LL + t.tv_nsec) / 1000;
            mPtr->test_count++;
            if(crtTime/1000000>mPtr->last_time/1000000){
                mPtr->last_time=crtTime;
                ALOGE("[fps=%d], last_time=%lld, socket recv.", mPtr->test_count,mPtr->last_time);
                mPtr->test_count = 0;
            }
        }
        //FlyZebra test code End
    }

    pthread_mutex_unlock(&mPtr->sendMutex);
    sendLen = send(socketID, CLOSE_CAMERA, sizeof(CLOSE_CAMERA), 0);
    pthread_mutex_unlock(&mPtr->sendMutex);
    if (sendLen <= 0) {
        ALOGE("socket send close camera error, sendLen=%d,socket=%d,errno=%d", sendLen, socketID, errno);
    }

    free(readBuf);
    ::close(socketID);
    mPtr->is_running = 0;
    ALOGE("socket recv thread exit......");
    return 0;
}

int FlySocket::convertFrame(void *frame, struct videoFrame *videoFrame, int format, uint32_t width, uint32_t height) {
    //ALOGE("read cameraID=%d, format=%d,width=%d,height=%d. Start", mCameraID, format, width, height);
    int f_width = videoFrame->width;
    int f_height = videoFrame->height;
    switch (format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            if(f_width==width && f_height==height && is_rotate==0){
                pthread_mutex_lock(&frameLock);
                libyuv::NV21ToNV12(
                    (const uint8_t *)videoFrame->data,
                    f_width,
                    (const uint8_t *)videoFrame->data + f_width * f_height,
                    f_width,
                    (uint8_t *)frame,
                    width,
                    (uint8_t *)frame + width * height,
                    width,
                    width,
                    height);
                pthread_mutex_unlock(&frameLock);
            }else{
                libyuv::NV21ToI420(
                    (const uint8_t *) videoFrame->data,
                    f_width,
                    (uint8_t *)videoFrame->data + f_width * f_height,
                    f_width,
                    temp1Data,
                    f_width,
                    temp1Data + f_width * f_height,
                    (f_width + 1) / 2,
                    temp1Data + f_width * f_height + f_width * f_height / 4,
                    (f_width + 1) / 2,
                    f_width,
                    f_height);

                if(is_rotate == 0){
                    if (f_height > f_width) {
                        libyuv::I420Rotate(
                            (const uint8_t *) temp1Data,
                            f_width,
                            temp1Data + f_width * f_height,
                            (f_width + 1) / 2,
                            temp1Data + f_width * f_height + f_width * f_height / 4,
                            (f_width + 1) / 2,
                            temp2Data,
                            f_height,
                            temp2Data + f_width * f_height,
                            (f_height + 1) / 2,
                            temp2Data + f_width * f_height + f_width * f_height / 4,
                            (f_height + 1) / 2,
                            f_width,
                            f_height,
                            libyuv::kRotate270);
                        std::swap(temp1Data, temp2Data);
                        std::swap(f_width, f_height);
                    }
                }else{
                    if (f_height > f_width) {
                        libyuv::I420Rotate(
                            (const uint8_t *) temp1Data,
                            f_width,
                            temp1Data + f_width * f_height,
                            (f_width + 1) / 2,
                            temp1Data + f_width * f_height + f_width * f_height / 4,
                            (f_width + 1) / 2,
                            temp2Data,
                            f_height,
                            temp2Data + f_width * f_height,
                            (f_height + 1) / 2,
                            temp2Data + f_width * f_height + f_width * f_height / 4,
                            (f_height + 1) / 2,
                            f_width,
                            f_height,
                            libyuv::kRotate90);
                        std::swap(temp1Data, temp2Data);
                        std::swap(f_width, f_height);
                    }else{
                        libyuv::I420Rotate(
                            (const uint8_t *) temp1Data,
                            f_width,
                            temp1Data + f_width * f_height,
                            (f_width + 1) / 2,
                            temp1Data + f_width * f_height + f_width * f_height / 4,
                            (f_width + 1) / 2,
                            temp2Data,
                            f_width,
                            temp2Data + f_width * f_height,
                            (f_width + 1) / 2,
                            temp2Data + f_width * f_height + f_width * f_height / 4,
                            (f_width + 1) / 2,
                            f_width,
                            f_height,
                            libyuv::kRotate180);
                        std::swap(temp1Data, temp2Data);
                    }
                }
                if (f_width != width || f_height != height)  {
                    libyuv::I420Scale(
                        (const uint8_t *) temp1Data,
                        f_width,
                        temp1Data + f_width * f_height,
                        (f_width + 1) / 2,
                        temp1Data + f_width * f_height + f_width * f_height / 4,
                        (f_width + 1) / 2,
                        f_width,
                        f_height,
                        temp2Data,
                        width,
                        temp2Data + width * height,
                        (width + 1) / 2,
                        temp2Data + width * height + width * height / 4,
                        (width + 1) / 2,
                        width,
                        height,
                        libyuv::kFilterNone);
                    std::swap(temp1Data, temp2Data);
            }
            pthread_mutex_lock(&frameLock);
            libyuv::I420ToNV12(
                (const uint8_t *) temp1Data,
                width,
                temp1Data + width * height,
                (width + 1) / 2,
                temp1Data + width * height + width * height / 4,
                (width + 1) / 2,
                (uint8_t *)frame,
                width,
                (uint8_t *)frame + width * height,
                width,
                width,
                height);
            }
            pthread_mutex_unlock(&frameLock);
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            memcpy(frame, videoFrame->data, f_width * f_height * 3 / 2);
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        case HAL_PIXEL_FORMAT_RGBA_8888:
        default:
            if(f_width==width && f_height==height && is_rotate==0){
                pthread_mutex_lock(&frameLock);
                libyuv::NV21ToARGB(
                    (const uint8_t *) videoFrame->data,
                    f_width,
                    (const uint8_t *)videoFrame->data + f_width * f_height,
                    f_width,
                    (uint8_t *)frame,
                    width * 4,
                    width,
                    height);
                pthread_mutex_unlock(&frameLock);
            }else{
                libyuv::NV21ToARGB(
                    (const uint8_t *) videoFrame->data,
                    f_width,
                    (const uint8_t *)videoFrame->data + f_width * f_height,
                    f_width,
                    temp1Data,
                    f_width * 4,
                    f_width,
                    f_height);
                if(is_rotate == 0){
                    if (f_height > f_width) {
                        libyuv::ARGBRotate(
                            (const uint8_t *) temp1Data,
                            f_width * 4,
                            temp2Data,
                            f_height * 4,
                            f_width,
                            f_height,
                            libyuv::kRotate270);
                        std::swap(temp1Data, temp2Data);
                        std::swap(f_width, f_height);
                    }
                }else{
                    if (f_height > f_width) {
                        libyuv::ARGBRotate(
                            (const uint8_t *) temp1Data,
                            f_width * 4,
                            temp2Data,
                            f_height * 4,
                            f_width,
                            f_height,
                            libyuv::kRotate90);
                        std::swap(temp1Data, temp2Data);
                        std::swap(f_width, f_height);
                    }else{
                        libyuv::ARGBRotate(
                            (const uint8_t *) temp1Data,
                            f_width * 4,
                            temp2Data,
                            f_width * 4,
                            f_width,
                            f_height,
                            libyuv::kRotate180);
                        std::swap(temp1Data, temp2Data);
                    }
                }
                if (f_width != width || f_height != height) {
                    libyuv::ARGBScale(
                        (const uint8_t *) temp1Data,
                        (f_height > f_width ? f_height : f_width) * 4,
                        f_height > f_width ? f_height : f_width,
                        f_height > f_width ? f_width : f_height,
                        temp2Data,
                        width * 4,
                        width,
                        height,
                        libyuv::kFilterNone);
                    std::swap(temp1Data, temp2Data);
                }
                pthread_mutex_lock(&frameLock);
                memcpy(frame, temp1Data, width * height * 4);
                pthread_mutex_unlock(&frameLock);
            }
            break;
    }
    //ALOGE("read CameraID=%d, format=%d,width=%d,height=%d. End", mCameraID, format, width, height);
    return 0;
}

int test_count1 = 0;
int test_count2 = 0;
int test_count3 = 0;
int64_t last_time1 = 0;
int64_t last_time2 = 0;
int64_t last_time3 = 0;
int FlySocket::readFrame(void *frame, int format, uint32_t width, uint32_t height) {
    //ALOGE("read cameraID=%d, format=%d,width=%d,height=%d. Start", mCameraID, format, width, height);
    struct timespec t = {.tv_sec = 0, .tv_nsec = 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    const int64_t crtTime = (t.tv_sec * 1000000000LL + t.tv_nsec) / 1000;
    pthread_mutex_lock(&frameLock);
    readWidth = width;
    readHeight = height;
    switch (format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            memcpy(frame, yuvFrame, width * height * 3 / 2);
            if(is_debug==1){
                test_count1++;
                if(crtTime/1000000>last_time1/1000000){
                    last_time1=crtTime;
                    ALOGE("[fps=%d], last_time=%lld, [%d]HAL_PIXEL_FORMAT_YCbCr_420_888", test_count1,last_time1,format);
                    test_count1 = 0;
                }
            }
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            memcpy(frame, yuvFrame, width * height * 3 / 2);
            if(is_debug==1){
                test_count2++;
                if(crtTime/1000000>last_time2/1000000){
                    last_time2=crtTime;
                    ALOGE("[fps=%d], last_time=%lld, [%d]HAL_PIXEL_FORMAT_BLOB", test_count2,last_time2,format);
                    test_count2 = 0;
                }
            }
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        case HAL_PIXEL_FORMAT_RGBA_8888:
        default:
            memcpy(frame, rgbFrame, width * height * 4);
            if(is_debug==1){
                test_count3++;
                if(crtTime/1000000>last_time3/1000000){
                    last_time3=crtTime;
                    ALOGE("[fps=%d], last_time=%lld, [%d]HAL_PIXEL_FORMAT_RGBA_8888", test_count3,last_time3,format);
                    test_count3 = 0;
                }
            }
            break;
    }
    pthread_mutex_unlock(&frameLock);
    //ALOGE("read CameraID=%d, format=%d,width=%d,height=%d. End", mCameraID, format, width, height);
    return 0;
}

//void FlySocket::memcpy(void *dst, void *src, int sz) {
//    if (sz & 63)
//            sz = (sz & -64) + 64;
//    asm volatile (
//    "NEONCopyPLD: \n"
//            " VLDM %[src]!,{d0-d7} \n"
//            " VSTM %[dst]!,{d0-d7} \n"
//            " SUBS %[sz],%[sz],#0x40 \n"
//            " BGT NEONCopyPLD \n"
//    : [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
//}


