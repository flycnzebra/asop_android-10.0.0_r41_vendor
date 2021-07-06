//
// Created by FlyZebra on 2021/1/13 0013.
//

#include "resample.h"
#include "flylog.h"

struct SwrContext* swr_cxt = NULL;

int sample_rate_init(int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate){
    FLYLOGE("%s", __func__);
    sample_rate_free();
    swr_cxt = swr_alloc();
	swr_alloc_set_opts(
		swr_cxt,
		out_ch_layout==AUDIO_CHANNEL_IN_MONO?AV_CH_LAYOUT_MONO:AV_CH_LAYOUT_STEREO,
		out_sample_fmt,
		out_sample_rate,
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_S16,
		48000,
		0,
		NULL);
	swr_init(swr_cxt);
	return 0;
}

int sample_rate_convert(uint8_t **swr_in_buf, uint8_t **swr_out_buf, int out_sample_rate, int64_t out_ch_layout){
    int64_t delay = swr_get_delay(swr_cxt, 48000);
    int64_t out_count = av_rescale_rnd(
            1024 + delay,
            out_sample_rate,
            48000,
            AV_ROUND_UP);
    int retLen = swr_convert(
            swr_cxt,
            (uint8_t **) swr_out_buf,
            out_count,
            (const uint8_t **) swr_in_buf,
            1024);
    if(DEBUG) FLYLOGD("swr_convert, delay=%lld, out_count=%lld, retLen=%d",delay, out_count, retLen);
    switch(out_ch_layout){
        case AUDIO_CHANNEL_IN_MONO:
            return retLen * 2;
        default:
            return retLen * 4;
    }
}

int sample_rate_free(){
    FLYLOGE("%s", __func__);
    if(swr_cxt != NULL) {
        swr_free(&swr_cxt);
        swr_cxt= NULL;
    }
    return 0;
}