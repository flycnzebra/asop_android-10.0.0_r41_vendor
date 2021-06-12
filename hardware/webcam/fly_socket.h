//
// Created by FlyZebra on 2020/9/2 0002.
//

#ifndef ANDROID_FLYSOCKET_H
#define ANDROID_FLYSOCKET_H

#include <pthread.h>
#include <cutils/properties.h>
struct videoFrame {
    uint16_t width;
	uint16_t height;
    uint64_t f_time;
	u_char data[4096*4096*3/2];
};

class FlySocket {
public:
    static FlySocket* getInstance();
    FlySocket();
    ~FlySocket();
    void open(int cameraID, int format, uint32_t width, uint32_t height);
    void close();
    int readFrame(void *frame, int format, uint32_t width, uint32_t height);
    int convertFrame(void *frame, struct videoFrame *videoFrame, int format, uint32_t width, uint32_t height);
    //void flymemcpy(void *dst, void *src, int sz);
private:
    static void *run(void *argv);
private:
    volatile int socketID;

    volatile int is_stop = 1;
    volatile int is_running = 0;
    volatile int is_empty = 1;
    pthread_t pid;

    pthread_cond_t frameCond;
    pthread_mutex_t frameLock;
    pthread_mutex_t sendMutex;

	struct videoFrame *videoFrame;
    u_char *temp1Data;
    u_char *temp2Data;

    u_char *yuvFrame;
    u_char *rgbFrame;

    volatile int readWidth;
    volatile int readHeight;

    const char *PROP_WEBCAM_ROTATE    = "persist.vendor.webcam.rotate";
    const char *PROP_WEBCAM_DEBUG     = "persist.vendor.webcam.debug";
    char webcam_rotate[PROPERTY_VALUE_MAX];
    char webcam_debug[PROPERTY_VALUE_MAX];
    volatile int is_rotate = 0;
    volatile int is_debug = 0;

    volatile int mCameraID = 0;

    int64_t last_time = 0;
    int test_count =  0;

};

static FlySocket* m_pInstance2 = NULL;


#endif //ANDROID_FLYSOCKET_H
