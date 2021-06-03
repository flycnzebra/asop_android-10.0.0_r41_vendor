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
cp -v $SRC/crosshatch/frameworks/base/Android._ $OBJ/frameworks/base/Android.bp
cp -v $SRC/crosshatch/frameworks/base/core/java/android/app/SystemServiceRegistry.java $OBJ/frameworks/base/core/java/android/app/SystemServiceRegistry.java
cp -v $SRC/crosshatch/frameworks/base/core/java/android/content/Context.java $OBJ/frameworks/base/core/java/android/content/Context.java
cp -v $SRC/crosshatch/frameworks/base/services/java/com/android/server/SystemServer.java $OBJ/frameworks/base/services/java/com/android/server/SystemServer.java
mkdir -p $OBJ/frameworks/base/core/java/android/zebra
cp -v $SRC/crosshatch/frameworks/base/core/java/android/zebra/FlyLog.java $OBJ/frameworks/base/core/java/android/zebra/FlyLog.java
cp -v $SRC/crosshatch/frameworks/base/core/java/android/zebra/IZebraService.aidl $OBJ/frameworks/base/core/java/android/zebra/IZebraService.aidl
cp -v $SRC/crosshatch/frameworks/base/core/java/android/zebra/ZebraListener.aidl $OBJ/frameworks/base/core/java/android/zebra/ZebraListener.aidl
cp -v $SRC/crosshatch/frameworks/base/core/java/android/zebra/ZebraManager.java $OBJ/frameworks/base/core/java/android/zebra/ZebraManager.java
mkdir -p $OBJ/frameworks/base/services/core/java/com/android/server/zebra
cp -v $SRC/crosshatch/frameworks/base/services/core/java/com/android/server/zebra/ZebraService.java $OBJ/frameworks/base/services/core/java/com/android/server/zebra/ZebraService.java
#make api-stubs-docs-update-current-api -j24
cp -v $SRC/crosshatch/frameworks/base/api/current.txt $OBJ/frameworks/base/api/current.txt

#copy crosshatch mk filse
cp -v $SRC/crosshatch/AndroidProducts._ $OBJ/device/google/crosshatch/AndroidProducts.mk
cp -v $SRC/crosshatch/aosp_crosshatch.mk $OBJ/device/google/crosshatch/
cp -v $SRC/crosshatch/device-crosshatch.mk $OBJ/device/google/crosshatch/

#create release-key
cp -rvf $SRC/crosshatch/build/target/product/security/* $OBJ/build/target/product/security/
cp -rvf $SRC/crosshatch/build/core/* $OBJ/build/core/
#All Selinux
cp -rvf $SRC/crosshatch/system/sepolicy/* $OBJ/system/sepolicy/

#gps
cp -v $SRC/crosshatch/frameworks/base/services/core/java/com/android/server/location/GnssLocationProvider.java $OBJ/frameworks/base/services/core/java/com/android/server/location/

#Launcher
cp -rvf $SRC/crosshatch/packages/apps/Launcher3 $OBJ/packages/apps/

echo "################<install zebra project finish>################"
