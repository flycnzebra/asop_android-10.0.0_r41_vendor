#LOCAL_PATH := $(call my-dir)
#include $(CLEAR_VARS)
#LOCAL_CFLAGS += \
#	-fno-short-enums -Wall -Wextra -Werror -fvisibility=hidden -DHAVE_JPEG \
#	-Wno-unused-parameter \
#	-Wno-format \
#	-Wno-shift-count-overflow \
#	-Wno-sign-compare
#LOCAL_SHARED_LIBRARIES := \
#	libbase \
#	libchrome \
#	libcamera_client \
#	libcamera_metadata \
#	libcutils \
#	libexif \
#	libhardware \
#	liblog \
#	libsync \
#	libutils \
#	libavcodec-57 \
#	libavdevice-57 \
#	libavfilter-6 \
#	libavformat-57 \
#	libavutil-55 \
#	libpostproc-54 \
#	libswresample-2 \
#	libswscale-4
#LOCAL_STATIC_LIBRARIES := \
#	libyuvz \
#	libjpeg_static_ndk
#LOCAL_SRC_FILES := \
#	arc/cached_frame.cpp \
#	arc/exif_utils.cpp \
#	arc/frame_buffer.cpp \
#	arc/image_processor.cpp \
#	arc/jpeg_compressor.cpp \
#	camera.cpp \
#	capture_request.cpp \
#	format_metadata_factory.cpp \
#	metadata/boottime_state_delegate.cpp \
#	metadata/enum_converter.cpp \
#	metadata/metadata.cpp \
#	metadata/metadata_reader.cpp \
#	request_tracker.cpp \
#	static_properties.cpp \
#	stream_format.cpp \
#	v4l2_camera.cpp \
#	v4l2_camera_hal.cpp \
#	v4l2_metadata_factory.cpp \
#	v4l2_wrapper.cpp \
#	fly_socket.cpp
#LOCAL_MODULE := camera.sdm845
#LOCAL_MODULE_RELATIVE_PATH := hw
#LOCAL_MODULE_TAGS := optional
#LOCAL_PROPRIETARY_MODULE := true
#include $(BUILD_SHARED_LIBRARY)
