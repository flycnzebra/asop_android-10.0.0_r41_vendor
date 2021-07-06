//
// Created by FlyZebra on 2021/1/13 0013.
//

#ifndef ANDROID_SAMPLE_RATE_H
#define ANDROID_SAMPLE_RATE_H


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <system/audio.h>

int sample_rate_init(int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate);
int sample_rate_convert(uint8_t **swr_in_buf, uint8_t **swr_out_buf,int out_sample_rate, int64_t out_ch_layout);
int sample_rate_free();

#endif //ANDROID_SAMPLE_RATE_H
