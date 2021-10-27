package com.android.server.zebra;

/**
 * @hide ClassName: Command
 * Author FlyZebra
 * 2021/10/27 0027 15:32
 * Describ:
 **/
public class Command {
    //t-->r
    //terminal connect heartbeat check
    //2byte	header EEAA
    //2byte	0101
    //4byte	data length
    //8byte	TID
    public final static byte HEARTBEAT_T[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x01, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x08,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
    };
    public final static int TYPE_HEARTBEAT_T = 0x0101;

    //r-->t
    //remote connect heartbeat check
    //2byte	header EEAA
    //2byte	0102
    //4byte	data length
    //8byte	UID
    public final static byte HEARTBEAT_R[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x01, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x08,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
    };
    public final static int TYPE_HEARTBEAT_R = 0x0201;

    //r-->t
    //remote connect heartbeat check
    //2byte	header EEAA
    //2byte	0102
    //4byte	data length
    //8byte	UID
    //......TID LIST
    public final static byte TERMINAL_LIST[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x01, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x08,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_TERMINAL_LIST = 0x0301;

    //r-->t
    //start video data
    //2byte	header EEAA
    //2byte	0201
    //4byte	data length
    //8byte TID
    //8byte UID
    //2byte width
    //2byte height
    //4byte bitrate/kbits
    public final static byte VIDEO_START[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x18,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_VIDEO_START = 0x0102;

    //r-->t
    //stop video data
    //2byte	header EEAA
    //2byte	0202
    //4byte	data length
    //8byte TID
    //8byte UID
    public final static byte VIDEO_STOP[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_VIDEO_STOP = 0x0202;

    //r-->t
    //start audio data
    //2byte	header EEAA
    //2byte	0203
    //4byte	data length
    //8byte TID
    //8byte UID
    //2byte sample_rate
    //2byte changle
    //4byte bitrate/kbits
    public final static byte AUDIO_START[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x18,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_AUDIO_START = 0x0302;

    //r-->t
    //stop audio data
    //2byte	header EEAA
    //2byte	0204
    //4byte	data length
    //8byte TID
    //8byte UID
    public final static byte AUDIO_STOP[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x04, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_AUDIO_STOP = 0x0402;

    //t-->r
    //video data
    //2byte	header EEAA
    //2byte	0211
    //4byte	data length
    //8byte	TID
    //4byte serial number
    //4byte time stamp
    public final static byte VIDEO_DATA[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x11, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_VIDEO_DATA = 0x1102;

    //t-->r
    //audio data
    //2byte	header EEAA
    //2byte	0212
    //4byte	data length
    //8byte	TID
    //4byte serial number
    //4byte time stamp
    public final static byte AUDIO_DATA[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x12, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_AUDIO_DATA = 0x1202;

    //t-->r
    //sps-pps data
    //2byte	header EEAA
    //2byte	0213
    //4byte	data length
    //8byte	TID
    //4byte serial number
    //4byte time stamp
    public final static byte SPSPPS_DATA[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x13, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_SPSPPS_DATA = 0x1302;

    //r-->t
    //video data connect check heartbeat
    //2byte	header EEAA
    //2byte	0214
    //4byte	data length
    //8byte TID
    //8byte UID
    public final static byte HEARTBEAT_VIDEO[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x14, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_HEARTBEAT_VIDEO = 0x1402;

    //r-->t
    //audio data connect check heartbeat
    //2byte	header EEAA
    //2byte	0215
    //4byte	data length
    //8byte TID
    //8byte UID
    public final static byte HEARTBEAT_AUDIO[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x02, (byte) 0x15, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_HEARTBEAT_AUDIO = 0x1502;

    //r-->t
    //input touch
    //2byte	header EEAA
    //2byte	0301
    //4byte	data length
    //8byte TID
    //1byte multi-touch sequence
    //1byte down (byte)0x00, move (byte)0x01, up (byte)0x02
    //2byte x
    //2byte y
    //2byte width
    //2byte heigh
    public final static byte INPUT_TOUCH[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x03, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x12,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_INPUT_TOUCH = 0x0103;

    //r-->t
    //input key
    //2byte	header EEAA
    //2byte	0302
    //4byte	data length
    //8byte TID
    //1byte down&up (byte)0x00, down (byte)0x01, up (byte)0x02
    //1byte key_vaule
    public final static byte INPUT_KEY[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x03, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x0A,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00
    };
    public final static int TYPE_INPUT_KEY = 0x0203;

    //r-->t
    //input text
    //2byte	header EEAA
    //2byte	0303
    //4byte	data length
    //8byte TID
    public final static byte INPUT_TEXT[] = {
            (byte) 0xEE, (byte) 0xAA, (byte) 0x03, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x08,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
    };
    public final static int TYPE_INPUT_TEXT = 0x0303;
}
