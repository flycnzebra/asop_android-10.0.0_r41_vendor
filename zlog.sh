#!/system/bin/sh

# Script debug Log
_DEBUG=OFF

# Script Log path
Script_log_path="/sdcard/zebralog.txt"

#========================================
LOGTOPPATH="/sdcard/zebralog"
LOGPATH="/sdcard/zebralog/log"
LOGCAT="$LOGPATH/logcat.txt"
LOGCAT_RADIO="$LOGPATH/logcat_radio.txt"
LOGCAT_EVENT="$LOGPATH/logcat_event.txt"
LOGDMESG="$LOGPATH/dmesg.txt"
LOGKMSG="$LOGPATH/kmsg.txt"
LOGCAT_SYSTEM="$LOGPATH/logcat_system.txt"
LOGCAT_CRASH="$LOGPATH/logcat_crash.txt"

LOGSYSPROP="$LOGPATH/sysprop.txt"
LOGMISCINFO="$LOGPATH/miscinfo.txt"

LOGTRACE="$LOGTOPPATH/trace"
#========================================

function DEBUG()
{
   [ "$_DEBUG" == "ON" ] && $@ || :
}

function DBG_MSG()
{
   DEBUG echo $1 >>$Script_log_path
}

function rename_log()
{
    log_file=
    index=
    tmp_file=

    DBG_MSG "========================"
    DBG_MSG "  Remove index 2 files  "
    DBG_MSG "========================"

    #cd "zebralog"
    cd $LOGPATH
    for file in `ls`
    do
      log_file=`echo $file`
      DBG_MSG $log_file
      index=`echo ${log_file:0:1}`
      DBG_MSG $index

      #index 2 file ,need to del
      if [[ 2 -eq $index ]] ; then
        rm -rf $log_file
      fi
    done

    DBG_MSG "============================="
    DBG_MSG "  Rename index 1 to 2 files  "
    DBG_MSG "============================="

    noIndexFile=
    tmp_file=

    for file in `ls`
    do
      log_file=`echo $file`
      DBG_MSG $log_file
      index=`echo ${log_file:0:1}`
      DBG_MSG $index

      #index 1 file ,need to rename
      if [[ 1 -eq $index ]] ; then
         noIndexFile=`echo ${log_file:1}`
         tmp_file=`echo "2"$noIndexFile`
         DBG_MSG $tmp_file
         mv $log_file  $tmp_file
      fi
    done

    DBG_MSG "========================"
    DBG_MSG "  Add index 1_ to files  "
    DBG_MSG "========================"

    for file in `ls`
    do
      log_file=`echo $file`
      DBG_MSG $log_file
      index=`echo ${log_file:0:1}`

      # index 1 file ,need to rename
      if [[ 1 -ne $index ]] && [[ 2 -ne $index ]] ; then
         tmp_file="1_$log_file"
         DBG_MSG $tmp_file
         mv $log_file  $tmp_file
      fi
    done
    DBG_MSG "all is ok"
}

function main()
{
   local boot_completed=`getprop sys.boot_completed`
   local download_mode

   while [ ! -d "/sdcard" ]
   do
     sleep 5;
   done

   DBG_MSG " "
   DBG_MSG " main corelog ++++ "

   if [[ 0 -eq $boot_completed ]] ; then
     # Android 6.0 is required to wait about 20 second
     sleep 20
   fi

  if [ ! -e $LOGPATH ] ; then
      mkdir -p $LOGPATH
   else
     rename_log
   fi

   DBG_MSG "copy anr log +++"
   #cp -rf /data/system/dropbox/  "$LOGTRACE/"
   mkdir -p $LOGTRACE
   cp -rf  "/data/anr/"  $LOGTRACE

   # Check crash event
   ramoops="/sys/fs/pstore/dmesg-ramoops-0"

   # Copy ramoops to zebralog then delete   
   cp -rf  /sys/fs/pstore/   "$LOGPATH/pstore"
   #rm -rf /sys/fs/pstore/*

   #/system/bin/logcat -r15000 -n 10 -v time -f $LOGCAT &
   #/system/bin/logcat -r5000 -n 4 -b radio -v time -f $LOGCAT_RADIO &
   #/system/bin/logcat -r5000 -n 5 -b events -v time -f $LOGCAT_EVENT &
   #/system/bin/logcat -r2000 -n 3 -b system -v time -f $LOGCAT_SYSTEM &
   #/system/bin/logcat -r5000 -n 3 -b events -v time -f $LOGCAT_EVENT &
   #/system/bin/logcat -r5000 -n 3 -b kernel -v time -f $LOGKMSG &
   #/system/bin/logcat -r1000 -n 2 -b crash -v time -f $LOGCAT_CRASH &
   /system/bin/logcat -r20000 -n 10 -b radio -b main -b system -b events -b kernel -b crash -v threadtime -f $LOGCAT
   #/system/bin/getprop > $LOGSYSPROP

   # Misc info
   df > $LOGMISCINFO
   echo -e "\n===========================">> $LOGMISCINFO
   cat /proc/meminfo >> $LOGMISCINFO
   echo -e "\n===========================">> $LOGMISCINFO
   cat /proc/cmdline >> $LOGMISCINFO

   DBG_MSG "corelog --- "

   exit  0
}

main