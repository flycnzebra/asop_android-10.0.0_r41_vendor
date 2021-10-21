LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# Module name should match apk name to be installed
LOCAL_MODULE := webcam
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com.flyzebra.player_debug_v1.10.0baeb52.202107031442_141.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
#LOCAL_PRIVILEGED_MODULE := true
#LOCAL_VENDOR_MODULE := true
#LOCAL_PROPRIETARY_MODULE := true
#LOCAL_DEX_PREOPT := false
LOCAL_PREBUILT_JNI_LIBS := \
lib/armeabi-v7a/libffplay.so \
lib/armeabi-v7a/libBugly.so \
lib/arm64-v8a/libffplay.so \
lib/arm64-v8a/libBugly.so
include $(BUILD_PREBUILT)