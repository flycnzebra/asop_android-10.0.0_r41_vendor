#!/bin/bash
#Copyright (c) 2021, flyzebra, flycnzebra@gmail.com #
. build/envsetup.sh
lunch aosp_crosshatch-userdebug
newversion=ZEBRA.DBUG.`date +%Y%m%d`.0`date +%H`
export BUILD_DISPLAY_ID=$newversion
make -j24
