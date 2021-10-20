//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef ANDROID_FLYLOG_H
#define ANDROID_FLYLOG_H

#include <utils/Log.h>

#define TAG "ZEBRA-VLTE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__)

#endif //ANDROID_FLYLOG_H
