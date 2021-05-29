LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Module name should match apk name to be installed
LOCAL_MODULE := mpc
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := xinwei.com.mpapp_release_v1.0.5b34cc9.202105290732_163.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := false
#LOCAL_DEX_PREOPT := false

include $(BUILD_PREBUILT)