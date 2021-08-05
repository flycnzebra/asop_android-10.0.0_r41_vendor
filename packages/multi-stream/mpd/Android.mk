LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := lib-mpd
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES := \
    mpd.c \
	mpd_rx.c \
	mpd_sack.c \
    mpd_schd.c \
	mpd_tx.c
LOCAL_CFLAGS := -Wno-return-type \
    -Wno-sign-compare \
    -Wno-missing-field-initializers \
    -Wno-implicit-function-declaration \
    -Wno-incompatible-pointer-types-discards-qualifiers \
    -Wno-parentheses \
    -Wno-format \
    -Wno-pointer-sign \
    -Wno-incompatible-function-pointer-types \
    -Wno-logical-op-parentheses \
    -Wno-int-conversion
include $(BUILD_SHARED_LIBRARY)