LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Module name should match apk name to be installed
LOCAL_MODULE := webcam
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com.flyzebra.player_release_v1.08.39692da.202105282027_105.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := false
#LOCAL_DEX_PREOPT := false
LOCAL_PREBUILT_JNI_LIBS := \
lib/armeabi-v7a/libavcodec-57.so \
lib/armeabi-v7a/libavdevice-57.so \
lib/armeabi-v7a/libavfilter-6.so \
lib/armeabi-v7a/libavformat-57.so \
lib/armeabi-v7a/libavutil-55.so \
lib/armeabi-v7a/libflyplayer.so \
lib/armeabi-v7a/libpostproc-54.so \
lib/armeabi-v7a/libswresample-2.so \
lib/armeabi-v7a/libswscale-4.so \
lib/armeabi-v7a/libBugly.so \
lib/arm64-v8a/libavcodec-57.so \
lib/arm64-v8a/libavdevice-57.so \
lib/arm64-v8a/libavfilter-6.so \
lib/arm64-v8a/libavformat-57.so \
lib/arm64-v8a/libavutil-55.so \
lib/arm64-v8a/libflyplayer.so \
lib/arm64-v8a/libpostproc-54.so \
lib/arm64-v8a/libswresample-2.so \
lib/arm64-v8a/libswscale-4.so \
lib/arm64-v8a/libBugly.so \

include $(BUILD_PREBUILT)