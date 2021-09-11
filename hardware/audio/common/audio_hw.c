/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include "protocol.h"
#include "flylog.h"
#include "resample.h"

//--------------------------add by flyzebra start

#define SERVER_IP "127.0.0.1"//127.0.0.1
#define SERVER_PORT "18183"//18183iqr pgy
#define RTMP_ID "0"
#define SOCKET_BUFFER  8192
#define PATH "fly_camera"
#define MAX_AUDIO_BUFFER 1920000

const char PROP_IP[] = "persist.vendor.audio.serverip";//声卡服务器IP地址
const char PROP_PROT[] = "persist.vendor.audio.serverport";//声卡服务器端口
const char PROP_WEBCAM[] = "persist.vendor.webcam.status";
const char PROP_VOICE[] = "persist.vendor.voice.status";
const char PROP_VOICE_SPEED[] = "persist.vendor.voice.speed";
const char PROP_RTMP_ID[] = "persist.vendor.rtmp.id";
char ip[PROPERTY_VALUE_MAX] = {0};
char port[PROPERTY_VALUE_MAX] = {0};
char webcam[PROPERTY_VALUE_MAX] = {0};
char voice[PROPERTY_VALUE_MAX] = {0};
char voice_speed[PROPERTY_VALUE_MAX] = {0};
char rtmpID[PROPERTY_VALUE_MAX] = {0};

//uint32_t play_sample_rate = 16000;//需要设置16000、44100.
//int play_buffer_size = 1280;//随采样率变化1280、4096
char send_buf[SOCKET_BUFFER] = {0};

//uint32_t in_sample_rate = 8000;//需要设置
//int in_buffer_size = 320;//随采样率变化

char audio_recv_buf[MAX_AUDIO_BUFFER] = {0};//audio 缓存
int audio_recv_buf_size = 0;//audio 缓存计数
char audio_send_buf[MAX_AUDIO_BUFFER] = {0};//audio 缓存
int audio_send_buf_size = 0;//audio 缓存计数

static pthread_mutex_t mutex_audio;
static pthread_cond_t cond_audio;

char recv_buf[SOCKET_BUFFER] = {0};

char tempbuf[SOCKET_BUFFER] = {0};

bool mic_state;
volatile int is_open_device = 0;
volatile int is_connect_cc = 0;
volatile int is_connect_wcam = 0;
volatile int recv_fail_count = 0;
volatile int send_fail_count = 0;
volatile int speak_count = 1;
volatile int is_open_speek = 0;
volatile int new_out_stream_count = 10;
volatile int is_open_wcam = 0;
volatile int out_stream_count = 0;
volatile int in_stream_count = 0;
volatile bool is_use_mic = true;
volatile bool is_use_camera = false;
volatile bool is_use_voice = false;
volatile int mic_speed = 8;

int socket_cc;
int socket_wcam;
pthread_t socket_threadid_cc;
pthread_t socket_threadid_wcam;
static pthread_mutex_t sendMutex_cc;
static pthread_mutex_t sendMutex_wcam;

uint8_t *swr_in_buf;
uint8_t *swr_out_buf;

void *ccSocketThread(void *argv);

void *wcamSocketThread(void *argv);

void sendOpenStream();

void sendCloseStream();

void sendOpenSpeak();

void sendCloseSpeak();

//--------------------------add by flyzebra end

struct stub_audio_device {
    struct audio_hw_device device;
};

struct stub_stream_out {
    struct audio_stream_out stream;
    audio_format_t format;
    audio_channel_mask_t channel_mask;
    uint32_t sample_rate;
    int buffer_size;
    int64_t last_write_time_us;
};
struct stub_stream_out *default_out;
struct stub_stream_out *current_out;

struct stub_stream_in {
    struct audio_stream_in stream;
    audio_format_t format;
    audio_channel_mask_t channel_mask;
    uint32_t sample_rate;
    int buffer_size;
    int64_t last_read_time_us;
};
struct stub_stream_in *default_in;
struct stub_stream_in *current_in;

static uint32_t out_get_sample_rate(const struct audio_stream *stream) {
    struct stub_stream_out *out = (struct stub_stream_out *) stream;
    if (DEBUG) FLYLOGD("out_get_sample_rate: %d.", out->sample_rate);
    return out->sample_rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate) {
    if (DEBUG) FLYLOGD("out_set_sample_rate: set rate=%d, return=%d.", rate, -ENOSYS);
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream) {
    struct stub_stream_out *out = (struct stub_stream_out *) stream;
    if (DEBUG) FLYLOGD("out_get_buffer_size: %d.", out->buffer_size);
    return out->buffer_size;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream) {
    if (DEBUG) FLYLOGD("out_get_channels: %d.", AUDIO_CHANNEL_OUT_STEREO);
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream) {
    if (DEBUG) FLYLOGD("out_get_format: %d.", AUDIO_FORMAT_PCM_16_BIT);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format) {
    if (DEBUG) FLYLOGD("out_set_format: set format=%s, return=%d.", format, -ENOSYS);
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream) {
    if (DEBUG) FLYLOGD("out_standby");
    struct stub_stream_out *out = (struct stub_stream_out *) (stream);
    out->last_write_time_us = 0;
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd) {
    if (DEBUG) FLYLOGD("out_dump: set fd=%d, return=%d.", fd, 0);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs) {
    if (DEBUG) FLYLOGD("out_set_parameters: set kvpairs=%s, return=%d.", kvpairs, 0);
    return 0;
}

static char *out_get_parameters(const struct audio_stream *stream, const char *keys) {
    if (DEBUG) FLYLOGD("out_get_parameters: set keys=%s.", keys);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream) {
    if (DEBUG) FLYLOGD("out_get_latency: return=%d.", 0);
    return 0;
}

static int out_set_volume(struct audio_stream_out *stream, float left, float right) {
    if (DEBUG) FLYLOGD("out_set_volume: Left:%f Right:%f. return=%d.", left, right, 1);
    return 1;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer, size_t bytes) {
    /* XXX: fake timing for audio output */
    struct stub_stream_out *out = (struct stub_stream_out *) (stream);
    const size_t frame_size = audio_stream_out_frame_size(stream);
    const uint32_t stream_sample_rate = out->sample_rate;
    struct timespec t = {.tv_sec = 0, .tv_nsec = 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    const int64_t now = (t.tv_sec * 1000000000LL + t.tv_nsec) / 1000;
    const int64_t elapsed_time_since_last_write = now - out->last_write_time_us;
    int64_t sleep_time =
            bytes * 1000000LL / frame_size / stream_sample_rate - elapsed_time_since_last_write;
    if (sleep_time > 0) {
        usleep(sleep_time);
    } else {
        // we don't sleep when we exit standby (this is typical for a real alsa buffer).
        sleep_time = 0;
    }
    out->last_write_time_us = now + sleep_time;
    // last_write_time_us is an approximation of when the (simulated) alsa
    // buffer is believed completely full. The usleep above waits for more space
    // in the buffer, but by the end of the sleep the buffer is considered
    // topped-off.
    //
    // On the subsequent out_write(), we measure the elapsed time spent in
    // the mixer. This is subtracted from the sleep estimate based on frames,
    // thereby accounting for drain in the alsa buffer during mixing.
    // This is a crude approximation; we don't handle underruns precisely.
    //fixed audio stream
    if (out != current_out) {
        new_out_stream_count++;
    } else {
        new_out_stream_count = 0;
    }
    if (new_out_stream_count >= 5) {
        current_out = out;
        sendOpenStream();
    }
    if (out != current_out) {
        return bytes;
    }
    if (is_connect_cc == 1 && bytes > 0 && buffer) {
        pthread_mutex_lock(&sendMutex_cc);
        uint32_t allLen = sizeof(PC_PLAY_DATA) + bytes - 4;
        send_head[2] = (allLen >> 24) & 0xFF;
        send_head[3] = (allLen >> 16) & 0xFF;
        send_head[4] = (allLen >> 8) & 0xFF;
        send_head[5] = allLen & 0xFF;

        send_head[10] = 0x01;
        send_head[11] = (((out->channel_mask==out->sample_rate?0x01:0x02)<<4) & 0xF0)|((out->format==AUDIO_FORMAT_PCM_16_BIT?0x02:0x01) & 0x0F);
        send_head[12] = (out->sample_rate>>8) & 0xFF;
        send_head[13] = out->sample_rate & 0xFF;

        send_head[14] = (bytes >> 24) & 0xFF;
        send_head[15] = (bytes >> 16) & 0xFF;
        send_head[16] = (bytes >> 8) & 0xFF;
        send_head[17] = bytes & 0xFF;
        int headLen = send(socket_cc, send_head, sizeof(send_head), MSG_DONTWAIT);
        if (headLen < 0) {
            send_fail_count++;
            pthread_mutex_unlock(&sendMutex_cc);
            FLYLOGE("socket send: headLen=%d,socket=%d,errno=%d,frame_size=%d,sample_rate=%d",
                    headLen, socket_cc, errno, frame_size, stream_sample_rate);
            return bytes;
        }
        int sendLen = send(socket_cc, buffer, bytes, MSG_DONTWAIT);
        if (sendLen < 0) {
            send_fail_count++;
            pthread_mutex_unlock(&sendMutex_cc);
            FLYLOGE("socket send: sendLen=%d,socket=%d,errno=%d,frame_size=%d,sample_rate=%d",
                    sendLen, socket_cc, errno, frame_size, stream_sample_rate);
            return bytes;
        }
        int tailLen = send(socket_cc, send_tail, 2, MSG_DONTWAIT);
        if (tailLen < 0) {
            send_fail_count++;
            pthread_mutex_unlock(&sendMutex_cc);
            FLYLOGE("socket send: tailLen=%d,socket=%d,errno=%d,frame_size=%d,sample_rate=%d",
                    tailLen, socket_cc, errno, frame_size, stream_sample_rate);
            return bytes;
        }
        pthread_mutex_unlock(&sendMutex_cc);
        if (DEBUG)
            FLYLOGD("socket send: dataLen=%d,sendLen=%d,socket=%d,errno=%d,frame_size=%d,sample_rate=%d",
                    bytes, (headLen + sendLen + tailLen), socket_cc, errno, frame_size,
                    stream_sample_rate);
    }
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream, uint32_t *dsp_frames) {
    *dsp_frames = 0;
    if (DEBUG) FLYLOGD("out_get_render_position: dsp_frames: %p", dsp_frames);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect) {
    if (DEBUG) FLYLOGD("out_add_audio_effect: %p", effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect) {
    if (DEBUG) FLYLOGD("out_remove_audio_effect: %p", effect);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream, int64_t *timestamp) {
    *timestamp = 0;
    if (DEBUG) FLYLOGD("out_get_next_write_timestamp: %ld", (long int) (*timestamp));
    return -EINVAL;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream) {
    struct stub_stream_in *in = (struct stub_stream_in *) stream;
    if (DEBUG) FLYLOGD("in_get_sample_rate: %d.", in->sample_rate);
    return in->sample_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate) {
    if (DEBUG) FLYLOGD("in_set_sample_rate: set rate=%d, return=%d.", rate, -ENOSYS);
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream) {
    struct stub_stream_in *in = (struct stub_stream_in *) stream;
    if (DEBUG) FLYLOGD("in_get_buffer_size: %d.", in->buffer_size);
    return in->buffer_size;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream) {
    struct stub_stream_in *in = (struct stub_stream_in *) stream;
    if (DEBUG) FLYLOGD("in_get_channels: %d.", in->channel_mask);
    return in->channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream) {
    if (DEBUG) FLYLOGD("in_get_format: return=%d.", AUDIO_FORMAT_PCM_16_BIT);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format) {
    if (DEBUG) FLYLOGD("in_set_format: set format=%s", format);
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream) {
    if (DEBUG) FLYLOGD("in_standby");
    struct stub_stream_in *in = (struct stub_stream_in *) stream;
    in->last_read_time_us = 0;
    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd) {
    if (DEBUG) FLYLOGD("in_dump: set fd=%d,return=%d.", fd, 0);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs) {
    if (DEBUG) FLYLOGD("in_set_parameters: set kvpairs=%s, return=%d.", kvpairs, 0);
    return 0;
}

static char *in_get_parameters(const struct audio_stream *stream, const char *keys) {
    if (DEBUG) FLYLOGD("in_get_parameters: set keys=%s.", keys);
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain) {
    if (DEBUG) FLYLOGD("in_set_gain: set gain=%f, return=%d.", gain, 0);
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void *buffer, size_t bytes) {
    if (DEBUG) FLYLOGD("in_read: bytes %zu", bytes);

    size_t retLen = bytes;

    speak_count = 5;
    memset(buffer, 0, bytes);
    if (is_open_speek == 0) {
        is_open_speek = 1;
        //open speak.
        sendOpenSpeak();
        //clear socket buffer
		int recvLen = recv(socket_cc, tempbuf, SOCKET_BUFFER, MSG_DONTWAIT);
        FLYLOGE("clear speak socket buffer(in_read), recvLen=%d,socket=%d,errno=%d", recvLen, socket_cc, errno);
        while (recvLen > 0) {
            recvLen = recv(socket_cc, tempbuf, SOCKET_BUFFER, MSG_DONTWAIT);
            FLYLOGE("clear speak cc socket buffer(in_read), recvLen=%d,socket=%d,errno=%d", recvLen, socket_cc, errno);
        }
        pthread_mutex_lock(&mutex_audio);
		recvLen = recv(socket_wcam, tempbuf, SOCKET_BUFFER, MSG_DONTWAIT);
        FLYLOGE("clear speak socket buffer(in_read), recvLen=%d,socket=%d,errno=%d", recvLen, socket_wcam, errno);
        while (recvLen > 0) {
            recvLen = recv(socket_wcam, tempbuf, SOCKET_BUFFER, MSG_DONTWAIT);
            FLYLOGE("clear speak wcam socket buffer(in_read), recvLen=%d,socket=%d,errno=%d", recvLen, socket_wcam, errno);
        }
        //clear audio buffer
        audio_recv_buf_size = 0;
        audio_send_buf_size = 0;
        pthread_mutex_unlock(&mutex_audio);
    }
    //struct stub_stream_out *out = (struct stub_stream_out *)stream;
    property_get(PROP_WEBCAM, webcam, "close");
    is_use_camera = (strncmp(webcam, "open", 4) == 0); 
    if (is_use_camera || is_use_voice) {
		//FLYLOGE("wcam write speak buffer, start.");
        is_open_wcam = 1;
        pthread_mutex_lock(&mutex_audio);
        if (!is_use_voice && (audio_send_buf_size < bytes)) {
            pthread_cond_wait(&cond_audio, &mutex_audio);
		}
		if (audio_send_buf_size >= bytes) {
			memcpy(buffer, audio_send_buf, bytes);
			audio_send_buf_size -= bytes;
			memmove(audio_send_buf, audio_send_buf + bytes, audio_send_buf_size);
			retLen = bytes;
		}else{
			memset(buffer, 0, bytes);
			retLen = 0;
		}
        pthread_mutex_unlock(&mutex_audio);
		//FLYLOGE("wcam write speak buffer, bytes=%d,audio_send_buf_size=%d.",retLen,audio_send_buf_size);
		//* XXX: fake timing for audio input */
	    struct stub_stream_in *in = (struct stub_stream_in *) stream;
	    struct timespec t = {.tv_sec = 0, .tv_nsec = 0};
	    clock_gettime(CLOCK_MONOTONIC, &t);
	    const int64_t now = (t.tv_sec * 1000000000LL + t.tv_nsec) / 1000;

	    // we do a full sleep when exiting standby.
	    const bool standby = in->last_read_time_us == 0;
	    const int64_t elapsed_time_since_last_read = standby ? 0 : now - in->last_read_time_us;
	    int64_t sleep_time = retLen * 1000000LL / audio_stream_in_frame_size(stream) /
	                         in_get_sample_rate(&stream->common) - elapsed_time_since_last_read;
	    //int64_t sleep_time = 20000LL - (now-in->last_read_time_us);
	    if(is_use_voice) sleep_time = sleep_time/mic_speed;
	    if (!is_use_camera && sleep_time > 0) {
	        usleep(sleep_time);
	        //FLYLOGE("usleep time=%lld, last time=%lld, now=%lld",sleep_time, (int64_t)in->last_read_time_us, now);
	    } else {
	        sleep_time = 0;
	    }
	    in->last_read_time_us = now + sleep_time;
	    // last_read_time_us is an approximation of when the (simulated) alsa
	    // buffer is drained by the read, and is empty.
	    //
	    // On the subsequent in_read(), we measure the elapsed time spent in
	    // the recording thread. This is subtracted from the sleep estimate based on frames,
	    // thereby accounting for fill in the alsa buffer during the interim.
    } else {
        is_open_wcam = 0;
        if (is_connect_cc == 1) {
            memset(recv_buf, 0, SOCKET_BUFFER);
            int recvLen = recv(socket_cc, recv_buf, SOCKET_BUFFER, MSG_DONTWAIT);
            if (recvLen < 6) {
                //FLYLOGE("recv data length errer 1!recvLen=%d,errno=%d",recvLen,errno);
                return 0;
            }
            long allLen = 0;
            if ((recv_buf[0] == (char) 0x7e) && (recv_buf[1] == (char) 0xa5)) {
                allLen = ((recv_buf[2] << 24) & 0xFF000000) + ((recv_buf[3] << 16) & 0xFF0000) +
                         ((recv_buf[4] << 8) & 0xFF00) + recv_buf[5];
            } else {
                FLYLOGE("recv head errer!recvLen=%d,head=%x%x", recv_buf[0] & 0xFF,
                        recv_buf[1] & 0xFF);
                return 0;
            }
            //if(allLen>in_buffer_size + sizeof(PC_SPEAK_DATA)){
            //    FLYLOGE("recv data length errer 2!allLen=%d,bytes=%d",allLen,bytes);
            //    return 0;
            //}
            while (recvLen < (allLen + 4)) {
                int ret = recv(socket_cc, recv_buf + recvLen, (allLen + 4 - recvLen), MSG_DONTWAIT);
                if (ret < 0) {
                    FLYLOGE("recv error!recvLen=%d,allLen=%d,ret=%d,errno=%d", recvLen, allLen, ret, errno);
                    return 0;
                } else {
                    recvLen += ret;
                }
            }
            if ((recv_buf[8] == (char) 0x04) && (recv_buf[9] == (char) 0x51)) {
                int dataLen =
                        ((recv_buf[14] << 24) & 0xFF000000) + ((recv_buf[15] << 16) & 0xFF0000) +
                        ((recv_buf[16] << 8) & 0xFF00) + recv_buf[17];
                if (dataLen == bytes) {
                    memcpy(buffer, recv_buf + sizeof(PC_SPEAK_DATA) - 2, bytes);
                } else if (dataLen > bytes) {
                    memcpy(buffer, recv_buf + sizeof(PC_SPEAK_DATA) - 2, bytes);
                    FLYLOGE("recv data length errer 3! dataLen=%d,bytes=%d", dataLen, bytes);
                    retLen = bytes;
                } else {
                    memcpy(buffer, recv_buf + sizeof(PC_SPEAK_DATA) - 2, dataLen);
                    FLYLOGE("recv data length errer 4! dataLen=%d,bytes=%d", dataLen, bytes);
                    retLen = dataLen;
                }
            } else if ((recv_buf[8] == (char) 0x04) && (recv_buf[9] == (char) 0x4e)) {
                sendOpenStream();
                return 0;
            } else {
                FLYLOGE("recv unkown message!recvLen=%d,head=%x%x,message=%x%x.",
                        recv_buf[0] & 0xFF, recv_buf[1] & 0xFF, recv_buf[8] & 0xFF,
                        recv_buf[9] & 0xFF);
                return 0;
            }
            if (DEBUG)
                FLYLOGD("cc socket recv: recvLen=%d,bytes=%d,socket=%d,errno=%d.", recvLen, bytes, socket_cc, errno);
        }
		//* XXX: fake timing for audio input */
	    struct stub_stream_in *in = (struct stub_stream_in *) stream;
	    struct timespec t = {.tv_sec = 0, .tv_nsec = 0};
	    clock_gettime(CLOCK_MONOTONIC, &t);
	    const int64_t now = (t.tv_sec * 1000000000LL + t.tv_nsec) / 1000;

	    // we do a full sleep when exiting standby.
	    const bool standby = in->last_read_time_us == 0;
	    const int64_t elapsed_time_since_last_read = standby ? 0 : now - in->last_read_time_us;
	    int64_t sleep_time = retLen * 1000000LL / audio_stream_in_frame_size(stream) /
	                         in_get_sample_rate(&stream->common) - elapsed_time_since_last_read;
	    //int64_t sleep_time = 20000LL - (now-in->last_read_time_us);
	    if (sleep_time > 0) {
	        usleep(sleep_time);
	        //FLYLOGE("usleep time=%lld, last time=%lld, now=%lld",sleep_time, (int64_t)in->last_read_time_us, now);
	    } else {
	        sleep_time = 0;
	    }
	    in->last_read_time_us = now + sleep_time;
	    // last_read_time_us is an approximation of when the (simulated) alsa
	    // buffer is drained by the read, and is empty.
	    //
	    // On the subsequent in_read(), we measure the elapsed time spent in
	    // the recording thread. This is subtracted from the sleep estimate based on frames,
	    // thereby accounting for fill in the alsa buffer during the interim.
	}    
    return retLen;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream) {
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect) {
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect) {
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused) {
    if (DEBUG) FLYLOGD("adev_open_output_stream...");

    out_stream_count++;
    *stream_out = NULL;
    struct stub_stream_out *out = (struct stub_stream_out *) calloc(1,
                                                                    sizeof(struct stub_stream_out));
    if (!out) return -ENOMEM;

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;

    //FLYLOGE("%s format1=%d, format2=%d", __func__, config->format, config->offload_info.format);
    //FLYLOGE("%s channel1=%d, channel2=%d", __func__, config->channel_mask, config->offload_info.channel_mask);
    //FLYLOGE("%s sample_rate1=%d, sample_rate2=%d", __func__, config->sample_rate, config->offload_info.sample_rate);
    out->format = AUDIO_FORMAT_PCM_16_BIT;
    out->channel_mask = config->offload_info.channel_mask;
    out->sample_rate = config->offload_info.sample_rate;
    if (out->sample_rate < 16000) {
        out->sample_rate = 16000;
    }
    switch(out->channel_mask){
        case AUDIO_CHANNEL_OUT_MONO:
            out->buffer_size = out->sample_rate * 40 / 1000;
            break;
        case AUDIO_CHANNEL_OUT_STEREO:
        default:
            out->buffer_size = out->sample_rate * 80 / 1000;
            break;
    }
    FLYLOGE("adev_open_output_stream[%d], sample_rate=%d, buffer_size=%d", out_stream_count, out->sample_rate, out->buffer_size);
    *stream_out = &out->stream;

    //sendOpenStream(out->sample_rate);
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream) {
    if (DEBUG) FLYLOGD("adev_close_output_stream...");
    out_stream_count--;
    struct stub_stream_out *out = (struct stub_stream_out *) stream;
    FLYLOGE("adev_close_output_stream[%d], format=%d, channel=%d, sample_rate=%d, buffer_size=%d",out_stream_count, out->format, out->channel_mask, out->sample_rate, out->buffer_size);
    //free(stream);
    free(out);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs) {
    if (DEBUG) FLYLOGD("adev_set_parameters %s", kvpairs);
    return -ENOSYS;
}

static char *adev_get_parameters(const struct audio_hw_device *dev, const char *keys) {
    if (DEBUG) FLYLOGD("adev_get_parameters");
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev) {
    if (DEBUG) FLYLOGD("adev_init_check");
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume) {
    if (DEBUG) FLYLOGD("adev_set_voice_volume: %f", volume);
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume) {
    if (DEBUG) FLYLOGD("adev_set_master_volume: %f", volume);
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume) {
    if (DEBUG) FLYLOGD("adev_get_master_volume: %f", *volume);
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted) {
    if (DEBUG) FLYLOGD("adev_set_master_mute: %d", muted);
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted) {
    if (DEBUG) FLYLOGD("adev_get_master_mute: %d", *muted);
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode) {
    if (DEBUG) FLYLOGD("adev_set_mode: %d", mode);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state) {
    if (DEBUG) FLYLOGD("adev_set_mic_mute: %d", state);
    mic_state = state;
    return mic_state;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state) {
    if (DEBUG) FLYLOGD("adev_get_mic_mute: %d", mic_state);
    return mic_state;
}

static size_t
adev_get_input_buffer_size(const struct audio_hw_device *dev, const struct audio_config *config) {
    //if (DEBUG) FLYLOGD("adev_get_input_buffer_size: %d", in_buffer_size);
    //return in_buffer_size;
     int sample_rate = config->sample_rate;
     if (sample_rate < 8000) {
         sample_rate = 8000;
     }
     switch(config->channel_mask){
         case AUDIO_CHANNEL_IN_STEREO:
             return sample_rate * 80 / 1000;
             break;
         default:
             return sample_rate * 40 / 1000;
             break;
     }
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused) {
    if (DEBUG) FLYLOGD("adev_open_input_stream...");
    in_stream_count++;

    *stream_in = NULL;
    struct stub_stream_in *in = (struct stub_stream_in *) calloc(1, sizeof(struct stub_stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    //FLYLOGE("%s format1=%d, format2=%d", __func__, config->format, config->offload_info.format);
    //FLYLOGE("%s channel1=%d, channel2=%d", __func__, config->channel_mask, config->offload_info.channel_mask);
    //FLYLOGE("%s sample_rate1=%d, sample_rate2=%d", __func__, config->sample_rate, config->offload_info.sample_rate);
    in->format = config->format;
    in->channel_mask = config->channel_mask;
    in->sample_rate = config->sample_rate;
    if (in->sample_rate < 8000) {
        in->sample_rate = 8000;
    }
    switch(in->channel_mask){
        case AUDIO_CHANNEL_IN_STEREO:
            in->buffer_size = in->sample_rate * 80 / 1000;
            break;
        default:
            in->buffer_size = in->sample_rate * 40 / 1000;
            break;
    }
    FLYLOGE("adev_open_input_stream[%d], sample_rate=%d, buffer_size=%d",in_stream_count, in->sample_rate, in->buffer_size);
    *stream_in = &in->stream;
    current_in = in;
    sample_rate_init(current_in->channel_mask, current_in->format, current_in->sample_rate);
	property_get(PROP_WEBCAM, webcam, "close");
    property_get(PROP_VOICE, voice, "mic");
	is_use_camera = (strncmp(webcam, "open", 4) == 0); 
	is_use_voice = (strncmp(voice, "voice", 5) == 0);
    is_use_mic = !is_use_camera && !is_use_voice;
	property_get(PROP_VOICE_SPEED, voice_speed, "8");
	mic_speed = atoi(voice_speed);
	if(mic_speed==0){
	    mic_speed = 1;
	}
    return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev, struct audio_stream_in *stream) {
    if (DEBUG) FLYLOGD("adev_close_input_stream...");
    in_stream_count--;
    if(in_stream_count<=0){
        sendCloseSpeak();
    }
    struct stub_stream_in *in = (struct stub_stream_in *) stream;
    FLYLOGE("adev_close_input_stream[%d], format=%d, channel=%d, sample_rate=%d, buffer_size=%d",in_stream_count, in->format, in->channel_mask, in->sample_rate, in->buffer_size);
    //free(stream);
    free(in);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd) {
    if (DEBUG) FLYLOGD("adev_dump");
    return 0;
}

static int adev_close(hw_device_t *device) {
    if (DEBUG) FLYLOGD("adev_close");
    is_open_device = 0;
    pthread_mutex_lock(&mutex_audio);
    pthread_cond_signal(&cond_audio);
    pthread_mutex_unlock(&mutex_audio);

    pthread_mutex_destroy(&sendMutex_cc);
    pthread_mutex_destroy(&sendMutex_wcam);
    pthread_mutex_destroy(&mutex_audio);
    pthread_cond_destroy(&cond_audio);
    free(device);
    free(default_out);
    free(default_in);
    return 0;
}

static int adev_open(const hw_module_t *module, const char *name, hw_device_t **device) {
    if (DEBUG) FLYLOGD("adev_open: %s", name);
    struct stub_audio_device *adev;
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) return -EINVAL;
    adev = calloc(1, sizeof(struct stub_audio_device));
    if (!adev) return -ENOMEM;
    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    *device = &adev->device.common;

    default_out = (struct stub_stream_out *) calloc(1, sizeof(struct stub_stream_out));
    default_out->format = AUDIO_FORMAT_PCM_16_BIT;
    default_out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    default_out->sample_rate = 16000;
    default_out->buffer_size = 1280;
    default_out->last_write_time_us = 0;
    current_out = default_out;

    default_in = (struct stub_stream_in *) calloc(1, sizeof(struct stub_stream_in));
    default_in->format = AUDIO_FORMAT_PCM_16_BIT;
    default_in->channel_mask = AUDIO_CHANNEL_IN_MONO;
    default_in->sample_rate = 8000;
    default_in->buffer_size = 320;
    default_in->last_read_time_us = 0;
    current_in = default_in;

    pthread_mutex_init(&sendMutex_cc, NULL);
    pthread_mutex_init(&sendMutex_wcam, NULL);
    pthread_mutex_init(&mutex_audio, NULL);
    pthread_cond_init(&cond_audio, NULL);

    is_open_device = 1;

    pthread_attr_t attr_cc;
    pthread_attr_init(&attr_cc);
    pthread_attr_setdetachstate(&attr_cc, PTHREAD_CREATE_DETACHED);
    int ret1 = pthread_create(&socket_threadid_cc, &attr_cc, ccSocketThread, NULL);

    pthread_attr_t attr_wcam;
    pthread_attr_init(&attr_wcam);
    pthread_attr_setdetachstate(&attr_wcam, PTHREAD_CREATE_DETACHED);
    int ret2 = pthread_create(&socket_threadid_wcam, &attr_wcam, wcamSocketThread, NULL);

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
        .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
                .hal_api_version = HARDWARE_HAL_API_VERSION,
                .id = AUDIO_HARDWARE_MODULE_ID,
                .name = "Default audio HW HAL",
                .author = "The Android Open Source Project",
                .methods = &hal_module_methods,
        },
};

//--------------------------add by flyzebra start
void sendOpenStream() {
    pthread_mutex_unlock(&sendMutex_cc);
    send_play[13] = atoi(rtmpID);
    send_play[14] = (current_out->sample_rate >> 8) & 0xFF;
    send_play[15] = (current_out->sample_rate) & 0xFF;
    int len = send(socket_cc, send_play, sizeof(send_play), MSG_DONTWAIT);
    pthread_mutex_unlock(&sendMutex_cc);
    FLYLOGE("start out stream, sample_rate=%d, sendLen=%d, errno=%d", current_out->sample_rate, len, errno);
}

void sendCloseStream() {
}

void sendOpenSpeak() {
    pthread_mutex_lock(&sendMutex_cc);
    open_speak[14] = (current_in->sample_rate >> 8) & 0xFF;
    open_speak[15] = (current_in->sample_rate) & 0xFF;
    switch(current_in->channel_mask){
        case AUDIO_CHANNEL_IN_STEREO:
            open_speak[17] = 0x02;
            break;
        default:
            open_speak[17] = 0x01;
            break;
    }
    open_speak[20] = (current_in->buffer_size >> 8) & 0xFF;
    open_speak[21] = (current_in->buffer_size) & 0xFF;
    int len = send(socket_cc, open_speak, sizeof(open_speak), MSG_DONTWAIT);
    pthread_mutex_unlock(&sendMutex_cc);
    FLYLOGE("open in stream cc, format=%d, channel=%d, sample_rate=%d, sendLen=%d, errno=%d.",
                                    current_in->format, current_in->channel_mask, current_in->sample_rate, len, errno);
    pthread_mutex_lock(&sendMutex_wcam);
    open_speak[14] = (current_in->sample_rate >> 8) & 0xFF;
    open_speak[15] = (current_in->sample_rate) & 0xFF;
    switch(current_in->channel_mask){
        case AUDIO_CHANNEL_IN_STEREO:
            open_speak[17] = 0x02;
            break;
        default:
            open_speak[17] = 0x01;
            break;
    }
    open_speak[20] = (current_in->buffer_size >> 8) & 0xFF;
    open_speak[21] = (current_in->buffer_size) & 0xFF;
    len = send(socket_wcam, open_speak, sizeof(open_speak), MSG_DONTWAIT);
    pthread_mutex_unlock(&sendMutex_wcam);
    FLYLOGE("open in stream wcam, format=%d, channel=%d, sample_rate=%d, sendLen=%d, errno=%d.",
                                    current_in->format, current_in->channel_mask, current_in->sample_rate, len, errno);
}

void sendCloseSpeak() {
    pthread_mutex_lock(&sendMutex_cc);
    int len = send(socket_cc, PC_CLOSE_SPEAK, sizeof(PC_CLOSE_SPEAK), MSG_DONTWAIT);
    pthread_mutex_unlock(&sendMutex_cc);
    FLYLOGE("close in stream cc, format=%d, channel=%d, sample_rate=%d, sendLen=%d, errno=%d.",
                                    current_in->format, current_in->channel_mask, current_in->sample_rate, len, errno);
    pthread_mutex_lock(&sendMutex_wcam);
    len = send(socket_wcam, PC_CLOSE_SPEAK, sizeof(PC_CLOSE_SPEAK), MSG_DONTWAIT);
    pthread_mutex_unlock(&sendMutex_wcam);
    FLYLOGE("close in stream wcam, format=%d, channel=%d, sample_rate=%d, sendLen=%d, errno=%d.",
                                    current_in->format, current_in->channel_mask, current_in->sample_rate, len, errno);
    pthread_mutex_lock(&mutex_audio);
    pthread_cond_signal(&cond_audio);
    pthread_mutex_unlock(&mutex_audio);
}

void *ccSocketThread(void *argv) {
    FLYLOGE("ccSocketThread start.");
    signal(SIGPIPE, SIG_IGN);

    memset(rtmpID, 0, PROPERTY_VALUE_MAX);
    property_get(PROP_RTMP_ID, rtmpID, RTMP_ID);

    while (is_open_device == 1) {
        while (is_connect_cc == 0) {
            FLYLOGE("try connect to cc server...");

            recv_fail_count = 0;
            send_fail_count = 0;

            memset(ip, 0, PROPERTY_VALUE_MAX);
            property_get(PROP_IP, ip, SERVER_IP);
            memset(port, 0, PROPERTY_VALUE_MAX);
            property_get(PROP_PROT, port, SERVER_PORT);

            socket_cc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            struct sockaddr_in servaddr;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(atoi(port));
            servaddr.sin_addr.s_addr = inet_addr(ip);
            if (connect(socket_cc, (struct sockaddr *) &servaddr, sizeof(servaddr)) == 0) {
                is_connect_cc = 1;
            } else {
                is_connect_cc = 0;
                close(socket_cc);
                sleep(1);
            }
            if (is_connect_cc == 1) {
                int flags = fcntl(socket_cc, F_GETFL, 0);
                fcntl(socket_cc, F_SETFL, flags | O_NONBLOCK);
                FLYLOGE("connect cc server success! ");
                sendOpenStream();
            }
        }
        if (send_fail_count > 5) {
            is_connect_cc = 0;
            close(socket_cc);
            FLYLOGE("send_fail_count>5 will reconnect server...");
        }
        speak_count--;
        if (is_open_speek == 1 && speak_count < -1) {
            is_open_speek = 0;
            //close speak
            //sendCloseSpeak();
            pthread_mutex_lock(&mutex_audio);
            pthread_cond_signal(&cond_audio);
            pthread_mutex_unlock(&mutex_audio);
        } else {
            if (is_connect_cc == 1) {
                int recvLen = recv(socket_cc, recv_buf, SOCKET_BUFFER, 0);
                if (recvLen >= 12) {
                    if ((recv_buf[0] == (char) 0x7e) && (recv_buf[1] == (char) 0xa5) &&
                        (recv_buf[8] == (char) 0x04) && (recv_buf[9] == (char) 0x4e)) {
                        sendOpenStream();
                    }
                }
            }
        }
        usleep(200000);
    }
    close(socket_cc);
    FLYLOGE("ccSocketThread exit.");
    return 0;
}

void *wcamSocketThread(void *argv) {
    FLYLOGE("wcamSocketThread start.");
    signal(SIGPIPE, SIG_IGN);
    swr_in_buf = (uint8_t *) av_malloc(48000 * 16);
    swr_out_buf = (uint8_t *) av_malloc(48000 * 16);
    sample_rate_init(current_in->channel_mask, current_in->format, current_in->sample_rate);
    while (is_open_device == 1) {
        //connect local socket
        if (is_connect_wcam == 0) {
            FLYLOGE("try connect to wcam server...");
            socket_wcam = socket_local_client("fly_camera", ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
            if (socket_wcam < 0) {
                FLYLOGE("wcam socket_local_client, socketID=%d", socket_wcam);
                sleep(5);
                continue;
            } else {
                is_connect_wcam = 1;
                int flags = fcntl(socket_wcam, F_GETFL, 0);
                fcntl(socket_wcam, F_SETFL, flags | O_NONBLOCK);
                FLYLOGE("connect wcam server success! ");

                //notify server this is audio client
                pthread_mutex_lock(&sendMutex_wcam);
                int sendLen = send(socket_wcam, OPEN_AUDIO, sizeof(OPEN_AUDIO), MSG_DONTWAIT);
                pthread_mutex_unlock(&sendMutex_wcam);
                FLYLOGE("send audio connect. sendLen=%d,socket=%d,errno=%d.", sendLen, socket_wcam, errno);
                if(sendLen != sizeof(OPEN_AUDIO)){
                    FLYLOGE("send audio connect failed!");
                    is_connect_wcam = 0;
                    close(socket_wcam);
                }
            }
        } else {
            int recvLen = recv(socket_wcam, audio_recv_buf + audio_recv_buf_size, MAX_AUDIO_BUFFER - audio_recv_buf_size, MSG_DONTWAIT);
            if (audio_recv_buf_size >= MAX_AUDIO_BUFFER) {
                if(DEBUG) FLYLOGD("audio buffer is full! audio_recv_buf_size=%d", audio_recv_buf_size);
                audio_recv_buf_size = 0;
            }
            if (recvLen > 0) {
                audio_recv_buf_size += recvLen;
            }
            if(audio_recv_buf_size > 4096){
                memcpy(swr_in_buf,audio_recv_buf, 4096);
                audio_recv_buf_size -= 4096;
                memmove(audio_recv_buf, audio_recv_buf + 4096, audio_recv_buf_size);
                int convertLen = sample_rate_convert(&swr_in_buf,&swr_out_buf,current_in->sample_rate,current_in->channel_mask);
                if(convertLen>0){
                    pthread_mutex_lock(&mutex_audio);
                    if ((audio_send_buf_size + convertLen) > MAX_AUDIO_BUFFER) {
                        FLYLOGE("wcam audio send buffer is full.");
                    }else{
                        memcpy(audio_send_buf + audio_send_buf_size,swr_out_buf, convertLen);
                        audio_send_buf_size += convertLen;
                    }
                    pthread_cond_signal(&cond_audio);
                    pthread_mutex_unlock(&mutex_audio);
                }
            }
            if (recvLen == 0) {
                is_connect_wcam = 0;
                close(socket_wcam);
                FLYLOGE("wcam socket recv error: recvLen=%d,socket=%d,errno=%d.", recvLen, socket_wcam, errno);
            } else if (recvLen < 0){
                if (errno == 11) {
                    usleep((is_open_wcam == 1 && is_open_speek == 1) ? 1000 : 10000);
					//FLYLOGE("is_open_wcam=%d,is_open_speek=%d, usleep.", is_open_wcam,is_open_speek);
                } else {
                    is_connect_wcam = 0;
                    close(socket_wcam);
                    FLYLOGE("wcam socket recv error: recvLen=%d,socket=%d,errno=%d.", recvLen, socket_wcam, errno);
                }
            }
            //usleep(5000);
        }
    }
    close(socket_wcam);
    sample_rate_free();
    av_free(swr_in_buf);
    av_free(swr_out_buf);
    FLYLOGE("wcamSocketThread exit.");
    return 0;
}
//--------------------------add by flyzebra end