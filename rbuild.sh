#!/bin/bash
#Copyright (c) 2021, tangshiyuan, flycnzebra@gmail.com #
android_dir=$(pwd)
packagedir=~/version/aosp_crosshatch
newversion=P3XL.DBUG.`date +%Y%m%d`.0`date +%H`
rm -rvf $packagedir/$newversion
for file in `ls $packagedir` 
do
	if [ -d $1"/"$file ]
		then filelist $1"/"$file 
	else
		oldversion=`basename $file` 
	fi
done
echo $oldversion
echo $newversion
. build/envsetup.sh
lunch aosp_crosshatch-userdebug
#第一个参数-c则执行make clean
if [ "$1" == "-c1" ];then
echo "find -c will make installclean project"
make installclean -j24
fi
if [ "$1" == "-c2" ];then
echo "find -c will make clean project"
make clean -j24
fi
make -j24
if [ ! -f "./out/target/product/crosshatch/system.img" ];then  
	echo "make error! not find system.img file!";
	exit 1;
fi
cd build/tools/signapk/
mm -j24
cd $android_dir
make otapackage -j24
if [ ! -f "./out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.tangshiyuan/IMAGES/system.img" ];then  
	echo "make error! not find ota system.img file!";
	exit 1;
fi
if [ ! -f "./out/target/product/crosshatch/aosp_crosshatch-ota-eng.tangshiyuan.zip" ];then 
	echo "make error! not find aosp_crosshatch-ota-eng.build.zip file!";
	exit 1;
fi
if [ ! -f "out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.tangshiyuan.zip" ];then 
	echo "make error! not find system.img aosp_crosshatch-target_files-eng.build.zip file!";
	exit 1;
fi
mkdir -p $packagedir/$newversion
cd out/target/product/
zip $packagedir/$newversion/$newversion.img.zip crosshatch/*.img
zip -u $packagedir/$newversion/$newversion.img.zip crosshatch/android-info.txt
cd $android_dir
cp -v out/target/product/crosshatch/aosp_crosshatch-ota-eng.tangshiyuan.zip $packagedir/$newversion/$newversion.ota.zip
cp -v out/target/product/crosshatch/obj/PACKAGING/target_files_intermediates/aosp_crosshatch-target_files-eng.tangshiyuan.zip $packagedir/$newversion/$newversion.target.zip
#差分包升级
./build/tools/releasetools/ota_from_target_files -v -2 -i $packagedir/$oldversion/$oldversion.target.zip $packagedir/$newversion/$newversion.target.zip $packagedir/$newversion/update_$oldversion-$newversion.zip
#差分包降级
./build/tools/releasetools/ota_from_target_files -v -2 -i $packagedir/$newversion/$newversion.target.zip $packagedir/$oldversion/$oldversion.target.zip $packagedir/$newversion/update_$newversion-$oldversion.zip
#cd ../../../
#rm -rf out-bin/
#source gen-out-bin.sh
#zip -r $packagedir/$newversion/$newversion.zip  out-bin/*
cd $packagedir/$newversion
find ./ -name "*.zip" | xargs -n1 md5sum > zmd5.txt
