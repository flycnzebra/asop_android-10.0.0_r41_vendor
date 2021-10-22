LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    reference-ril.c \
    atchannel.c \
    misc.c \
    at_tok.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libril librilutils
LOCAL_CFLAGS := -D_GNU_SOURCE
LOCAL_CFLAGS += -Wno-unused-variable 
LOCAL_CFLAGS += -Wno-unused-function
LOCAL_CFLAGS += -Wunused-parameter
LOCAL_VENDOR_MODULE:= true
LOCAL_CFLAGS += -DRIL_SHLIB
LOCAL_MODULE:= libvlte
include $(BUILD_SHARED_LIBRARY)
