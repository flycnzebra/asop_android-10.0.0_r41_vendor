LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES :=  \
    libcutils \
    libdl \
    liblog \
    libutils
LOCAL_SRC_FILES := 	time.cpp
LOCAL_MODULE := time
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    mp_data.c \
    mpcd.c \
    tun_dev.c \
    netlib.c \
    dataparse.c \
    mcwill_event.c \
    record_stream.c \
    plog.c \
    queue.c \
    ifaddrs/ifaddrs.c
LOCAL_CFLAGS += -Wno-sign-compare \
    -Wno-missing-field-initializers \
    -Wno-implicit-function-declaration \
    -Wno-incompatible-pointer-types-discards-qualifiers \
    -Wno-parentheses \
    -Wno-format \
    -Wno-pointer-sign \
    -Wno-incompatible-function-pointer-types \
    -Wno-logical-op-parentheses \
    -Wno-int-conversion
LOCAL_SHARED_LIBRARIES := libcutils liblog libcrypto libdl  libutils
LOCAL_STATIC_LIBRARIES := time
LOCAL_STRIP_MODULE := false
LOCAL_MODULE := ratd
include $(BUILD_EXECUTABLE)

