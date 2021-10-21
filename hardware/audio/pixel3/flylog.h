//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef ANDROID_FLYLOG_H
#define ANDROID_FLYLOG_H

#include <utils/Log.h>

#define     DEBUG           0
#define     TAG             "ZEBRA-AUDIO"

#define     FLYLOGD(...)    __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__)
#define     FLYLOGI(...)    __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#define     FLYLOGW(...)    __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__)
#define     FLYLOGE(...)    __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__)

#endif //ANDROID_FLYLOG_H
