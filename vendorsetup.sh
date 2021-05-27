#!/bin/bash
# Copyright (c) 2021, tangshiyuan, flycnzebra@gmail.com
echo "################<install zebra project start>################"
# set project dir
SRC=vendor/zebra
OBJ=.
#MYDIR=.
#TODIR=vendor/zebra 

#copy rbuild 
cp -rvf $SRC/rbuild.sh $OBJ/

#copy crosshatch mk filse
cp -v $SRC/AndroidProducts._ $OBJ/device/google/crosshatch/AndroidProducts.mk
cp -v $SRC/device-crosshatch.mk $OBJ/device/google/crosshatch/

#create release-key
cp -rvf $SRC/build/target/product/security/* $OBJ/build/target/product/security/
cp -rvf $SRC/build/core/* $OBJ/build/core/
cp -rvf $SRC/system/sepolicy/* $OBJ/system/sepolicy/
echo "################<install zebra project finish>################"
