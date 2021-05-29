LOCAL_PATH:= $(call my-dir)
out_dir := $(TARGET_OUT)/../vendor/rfs/msm/mpss/readonly/vendor/mbn/mcfg_sw
mk_dir := $(shell mkdir -p $(out_dir))
include $(CLEAR_VARS)
LOCAL_MODULE := mbn_sw.txt
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(out_dir)
LOCAL_SRC_FILES := mbn_sw.txt
include $(BUILD_PREBUILT)