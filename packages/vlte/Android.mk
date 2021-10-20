LOCAL_PATH:= $(call my-dir)

#vlte pro#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	 utils/wifi.c \
	 utils/wifi_fst.c \
     utils/dhcp_utils.c \
     vlte.cpp \
     network/BaseNetwork.cpp \
     network/WifiNetwork.cpp \
     network/UsbNetwork.cpp \

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_CFLAGS += -Wno-sign-compare -Wno-missing-field-initializers
LOCAL_SHARED_LIBRARIES := libcutils liblog libcrypto libdl libnetutils
LOCAL_MODULE := vlte
include $(BUILD_EXECUTABLE)


