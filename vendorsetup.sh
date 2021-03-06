#!/bin/bash
# Copyright (c) 2021, tangshiyuan, flycnzebra@gmail.com
echo "################<install zebra project start>################"
# set project dir
SRC=vendor/zebra
OBJ=.
#MYDIR=.
#TODIR=vendor/zebra 
#copy rbuild 
cp -v $SRC/rbuild.sh $OBJ/
#ZebrasSystemServer
cp -v $SRC/blueline/frameworks/base/Android._ $OBJ/frameworks/base/Android.bp
cp -v $SRC/blueline/frameworks/base/services/core/Android._ $OBJ/frameworks/base/services/core/Android.bp
cp -v $SRC/blueline/frameworks/base/core/java/android/app/SystemServiceRegistry.java $OBJ/frameworks/base/core/java/android/app/SystemServiceRegistry.java
cp -v $SRC/blueline/frameworks/base/core/java/android/content/Context.java $OBJ/frameworks/base/core/java/android/content/Context.java
cp -v $SRC/blueline/frameworks/base/services/java/com/android/server/SystemServer.java $OBJ/frameworks/base/services/java/com/android/server/SystemServer.java
mkdir -p $OBJ/frameworks/base/core/java/android/zebra
cp -v $SRC/blueline/frameworks/base/core/java/android/zebra/FlyLog.java $OBJ/frameworks/base/core/java/android/zebra/FlyLog.java
cp -v $SRC/blueline/frameworks/base/core/java/android/zebra/IZebraService.aidl $OBJ/frameworks/base/core/java/android/zebra/IZebraService.aidl
cp -v $SRC/blueline/frameworks/base/core/java/android/zebra/IZebraProcessService.aidl $OBJ/frameworks/base/core/java/android/zebra/IZebraProcessService.aidl
cp -v $SRC/blueline/frameworks/base/core/java/android/zebra/ZebraListener.aidl $OBJ/frameworks/base/core/java/android/zebra/ZebraListener.aidl
cp -v $SRC/blueline/frameworks/base/core/java/android/zebra/ZebraManager.java $OBJ/frameworks/base/core/java/android/zebra/ZebraManager.java
mkdir -p $OBJ/frameworks/base/services/core/java/com/android/server/zebra
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/zebra/Command.java $OBJ/frameworks/base/services/core/java/com/android/server/zebra/Command.java
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/zebra/ZebraService.java $OBJ/frameworks/base/services/core/java/com/android/server/zebra/ZebraService.java
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/zebra/ZebraProcessService.java $OBJ/frameworks/base/services/core/java/com/android/server/zebra/ZebraProcessService.java
#make api-stubs-docs-update-current-api -j24
cp -v $SRC/blueline/frameworks/base/api/current.txt $OBJ/frameworks/base/api/current.txt
#copy blueline mk files
cp -v $SRC/blueline/AndroidProducts._ $OBJ/device/google/crosshatch/AndroidProducts.mk
cp -v $SRC/blueline/aosp_blueline.mk $OBJ/device/google/crosshatch/
cp -v $SRC/blueline/device-blueline.mk $OBJ/device/google/crosshatch/
#create release-key
cp -rvf $SRC/blueline/build/make/target/product/security/* $OBJ/build/make/target/product/security/
cp -rvf $SRC/blueline/build/core/* $OBJ/build/core/
#init.rc
cp -v $SRC/blueline/system/core/rootdir/init.rc $OBJ/system/core/rootdir/
#wifi verity
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/ConnectivityService.java $OBJ/frameworks/base/services/core/java/com/android/server/
#gps
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/location/GnssLocationProvider.java $OBJ/frameworks/base/services/core/java/com/android/server/location/
#Launcher
cp -rvf $SRC/blueline/packages/apps/Launcher3 $OBJ/packages/apps/
#sepolicy
#cp -v $SRC/blueline/system/sepolicy/vendor/file_contexts $OBJ/system/sepolicy/vendor/
cp -v $SRC/blueline/device/google/crosshatch/BoardConfig-common._ $OBJ/device/google/crosshatch/BoardConfig-common.mk
#close selinux
cp -v $SRC/blueline/system/core/init/selinux.cpp $OBJ/system/core/init/
#multi-stream
cp -v $SRC/blueline/frameworks/base/core/res/AndroidManifest.xml $OBJ/frameworks/base/core/res/
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/ConnectivityService.java $OBJ/frameworks/base/services/core/java/com/android/server/
cp -v $SRC/blueline/frameworks/base/services/core/java/com/android/server/connectivity/ZebraVpn.java $OBJ/frameworks/base/services/core/java/com/android/server/connectivity/
#zebra hidl server
cp -v $SRC/blueline/system/libhidl/vintfdata/manifest.xml $OBJ/system/libhidl/vintfdata/
cp -rvf $SRC/blueline/hardware/interfaces/zebra $OBJ/hardware/interfaces/
cp -rvf $SRC/blueline/build/make/target/product/gsi $OBJ/build/make/target/product/
mv -v $OBJ/hardware/interfaces/zebra/Android._ $OBJ/hardware/interfaces/zebra/Android.bp
mv -v $OBJ/hardware/interfaces/zebra/1.0/Android._ $OBJ/hardware/interfaces/zebra/1.0/Android.bp
mv -v $OBJ/hardware/interfaces/zebra/1.0/default/Android._ $OBJ/hardware/interfaces/zebra/1.0/default/Android.bp
#?????????Android10????????????llkd
mv -v $OBJ/system/core/llkd/Android.bp $OBJ/system/core/llkd/Android._
mv -v $OBJ/system/core/llkd/tests/Android.bp $OBJ/system/core/llkd/tests/Android._
#??????????????????audio
cp -v $SRC/blueline/hardware/qcom/audio/Android._ $OBJ/hardware/qcom/audio/Android.mk
mv -v $OBJ/hardware/qcom/audio/hal/Android.mk $OBJ/hardware/qcom/audio/hal/Android._
echo "################<install zebra project finish>################"
