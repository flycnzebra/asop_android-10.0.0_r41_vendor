//
// Created by Administrator on 2020/3/14.
//

#ifndef ANDROID_FLYLOG_H
#define ANDROID_FLYLOG_H

#include <android/log.h>
#include <utils/Log.h>
#define FLY_TAG "ZEBRA---"
#define FLYLOGD(...) __android_log_print(ANDROID_LOG_DEBUG,FLY_TAG ,__VA_ARGS__)
#define FLYLOGI(...) __android_log_print(ANDROID_LOG_INFO,FLY_TAG ,__VA_ARGS__)
#define FLYLOGW(...) __android_log_print(ANDROID_LOG_WARN,FLY_TAG ,__VA_ARGS__)
#define FLYLOGE(...) __android_log_print(ANDROID_LOG_ERROR,FLY_TAG ,__VA_ARGS__)
#define FLYLOGF(...) __android_log_print(ANDROID_LOG_FATAL,FLY_TAG ,__VA_ARGS__)

#endif //ANDROID_FLYLOG_H
