LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Module name should match apk name to be installed
LOCAL_MODULE := fota
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com.flyzebra.fota_release_v1.03.2729378.202105290914_37.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := false
#LOCAL_DEX_PREOPT := false
LOCAL_PREBUILT_JNI_LIBS := \
lib/armeabi-v7a/libBugly.so \
lib/arm64-v8a/libBugly.so \

include $(BUILD_PREBUILT)