#!/bin/bash
#Copyright (c) 2021, flyzebra, flycnzebra@gmail.com #
. build/envsetup.sh
lunch aosp_crosshatch-userdebug

echo "################<create out dir>################"
android_dir=$(pwd)
packagedir=~/version/aosp_crosshatch
newversion=ZEBRA.DBUG.`date +%Y%m%d`.0`date +%H`
export BUILD_DISPLAY_ID=$newversion
rm -rvf $packagedir/$newversion
for file in `ls $packagedir`
do
	if [ -d $1"/"$file ]
		then filelist $1"/"$file
	else
		oldversion=`basename $file`
	fi
done
echo "$oldversion"-"$newversion"

if [ "$1" == "-c1" ];then
echo "################<make installclean>################"
make installclean -j40
fi

if [ "$1" == "-c2" ];then
echo "################<make clean>################"
make clean -j40
fi

echo "################<make project>################"
make -j40
if [ ! -f "./out/target/product/crosshatch/system.img" ];then
	echo "make error! not find system.img file!";
	exit 1;
fi

echo "################<make signapk>################"
cd build/tools/signapk/
mm -j40
cd $android_dir

echo "################<make otapackage>################"
make otapackage -j40
if [ ! -f "./out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.flyzebra/IMAGES/system.img" ];then
	echo "make error! not find ota system.img file!";
	exit 1;
fi
if [ ! -f "./out/target/product/crosshatch/aosp_crosshatch-ota-eng.flyzebra.zip" ];then
	echo "make error! not find aosp_crosshatch-ota-eng.build.zip file!";
	exit 1;
fi
if [ ! -f "out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.flyzebra.zip" ];then
	echo "make error! not find system.img aosp_crosshatch-target_files-eng.build.zip file!";
	exit 1;
fi

echo "################<out all image>################"
mkdir -p $packagedir/$newversion
cd out/target/product/
zip $packagedir/$newversion/$newversion.img.zip crosshatch/*.img
zip -u $packagedir/$newversion/$newversion.img.zip crosshatch/android-info.txt
cd $android_dir

echo "################<out ota file>################"
cp -v out/target/product/crosshatch/aosp_crosshatch-ota-eng.flyzebra.zip $packagedir/$newversion/$newversion.ota.zip

echo "################<out target file>################"
cp -v out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.flyzebra.zip $packagedir/$newversion/$newversion.target.zip

echo "################<up ota package>################"
./build/tools/releasetools/ota_from_target_files -i $packagedir/$oldversion/$oldversion.target.zip $packagedir/$newversion/$newversion.target.zip $packagedir/$newversion/update_$oldversion-$newversion.zip

echo "################<down ota package>################"
./build/tools/releasetools/ota_from_target_files -i $packagedir/$newversion/$newversion.target.zip $packagedir/$oldversion/$oldversion.target.zip $packagedir/$newversion/update_$newversion-$oldversion.zip

echo "################<md5sum files>################"
cd $packagedir/$newversion
find ./ -name "*.zip" | xargs -n1 md5sum > zmd5.txt
exit 0
