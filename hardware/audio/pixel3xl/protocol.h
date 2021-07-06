//
// Created by FlyZebra on 2020/7/30 0030.
//

#ifndef ANDROID_PROTOCOL_H
#define ANDROID_PROTOCOL_H

#if __cplusplus
extern "C" {
#endif
//音频播放接口
//准备播放接口（手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x044a
//
//采样率		2字节	Unsigned Integer	采样率，单位Hz，如：16000Hz
//通道数		2字节	Unsigned Integer	单声道：1 双声道：   2
//音频格式	2字节	Unsigned Integer	2：ENCODING_PCM_16BIT 3：ENCODING_PCM_8BIT 4：ENCODING_PCM_FLOAT
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_START_PLAY[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x12, 0x00, 0x02,
            0x04, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x80,
            0x00, 0x02, 0x00, 0x02, 0x7e, 0x0d};
u_char send_play[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x12, 0x00, 0x02,
            0x04, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x80,
            0x00, 0x02, 0x00, 0x02, 0x7e, 0x0d};

//音频数据传输接口（手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x044b
//音频路	    1字节	Unsigned Byte	支持多路音频
//通道数	    0.5字节	Unsigned Integer	单声道：1 双声道：2
//音频格式	0.5字节	Unsigned Integer	1：ENCODING_PCM_8BIT 2：ENCODING_PCM_16BIT
//采样率	    2字节	Unsigned Integer	采样率，单位Hz，如：16000Hz
//数据长度	4字节	Unsigned Integer	data len
//数据	x	byte	音频数据
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_PLAY_DATA[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x4b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x7e, 0x0d};
u_char send_head[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x4b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00};
u_char send_tail[] = {
            0x7e, 0x0d};

//暂停播放接口（手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x044c
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_PAUSE_PLAY[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02,
            0x04, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//播放完成接口 （手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x044d
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_STOP_PLAY[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02,
            0x04, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//音频录制接口
//准备录音接口 （手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0450
//
//采样率		2字节	Unsigned Integer	采样率，单位Hz，如：16000Hz
//通道数		2字节	Unsigned Integer	单声道：1 双声道：2
//音频格式	2字节	Unsigned Integer	2：ENCODING_PCM_16BIT 3：ENCODING_PCM_8BIT 4：ENCODING_PCM_FLOAT
//缓冲大小	2字节	Unsigned Integer	缓冲字节数
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_OPEN_SPEAK[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x50, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x40,
            0x00, 0x01, 0x00, 0x02, 0x01, 0x40, 0x7e, 0x0d};
u_char open_speak[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x50, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x40,
            0x00, 0x01, 0x00, 0x02, 0x01, 0x40, 0x7e, 0x0d};

//音频数据接口（PC—>手机 ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0451
//
//数据长度	4字节	Unsigned Integer	data len
//数据	x	byte	音频数据
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_SPEAK_DATA[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x10, 0x00, 0x02,
            0x04, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x7e, 0x0d};

//录音结束接口（手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0452
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char PC_CLOSE_SPEAK[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02,
            0x04, 0x52, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//准备播放接口 （CC-->HIL ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x044e
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
//音频录制接口
const static u_char PC_REPLAY_DATA[] =  {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02,
            0x04, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//摄像头打开消息
//准备录音接口 （手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0480
//预留字节	4字节	Unsigned Integer    预留字节
//宽度		2字节	Unsigned Integer	1280
//高度		2字节	Unsigned Integer	720
//视频格式	2字节	Unsigned Integer
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char OPEN_CAMERA[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//摄像头关闭消息
//准备录音接口 （手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0481
//预留字节	4字节	Unsigned Integer    预留字节
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char CLOSE_CAMERA[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x81, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//视频数据,只发送YUV420SP（NV12）数据
//准备录音接口 （手机-->PC ）
//开始符号	2字节	Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节	Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节	Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节	Unsigned Integer	命令号0x0482
//预留字节	4字节	Unsigned Integer    预留字节
//视频格式   2字节	Unsigned Integer    1 nv21
//宽度		2字节	Unsigned Integer	1280
//高度		2字节	Unsigned Integer	720
//时间   	8字节	Unsigned Integer
//结束符号	2字节	Unsigned Integer	固定的结束符号0x7e0d
const static u_char VIDEO_DATA[] = {
            0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
            0x04, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};


//连接声卡握手成功
//准备录音接口 （手机-->PC ）
//开始符号	2字节 Unsigned Integer	固定的开始符号0x7ea5
//消息长度	4字节 Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节 Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID	2字节 Unsigned Integer	命令号0x0483
//预留字节	4字节 Unsigned Integer	预留字节
//结束符号	2字节 Unsigned Integer	固定的结束符号0x7e0d
const static u_char OPEN_AUDIO[] = {
			0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
			0x04, 0x83, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};

//音频数据,只发送PCM数据
//准备录音接口 （手机-->PC ）
//开始符号	2字节 Unsigned Integer	固定的开始符号(byte)0x7ea5
//消息长度	4字节 Unsigned Integer	消息总长度，不包括开始和结束符
//协议版本	2字节 Unsigned Integer	协议版本号，高位在前，从1开始
//消息ID		2字节 Unsigned Integer	命令号0x0485
//预留字节	4字节    Unsigned Integer 预留字节
//音频路 	2字节 Unsigned Byte	支持多路音频
//通道数 	1字节 Unsigned Integer	单声道：1 双声道：2
//音频格式	1字节 Unsigned Integer	1：ENCODING_PCM_8BIT 2：ENCODING_PCM_16BIT
//采样率 	2字节 Unsigned Integer	采样率，单位Hz，如：16000Hz
//时间	8字节 Unsigned Integer
//数据	x	byte	音频数据
//结束符号	2字节 Unsigned Integer	固定的结束符号0x7e0d
const static u_char AUDIO_DATA[] = {
			0x7e, 0xa5, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02,
			0x04, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x02, 0x02, 0x80, 0x3e, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x7e, 0x0d};


#if __cplusplus
};  // extern "C"
#endif
#endif //ANDROID_PROTOCOL_H
