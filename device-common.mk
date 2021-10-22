#support china mobile sim
PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ims_support=1
PRODUCT_PROPERTY_OVERRIDES += ro.mtk_volte_support=1
PRODUCT_PROPERTY_OVERRIDES += persist.mtk.volte.enable=1
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.volte_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.ims_volte_enable=1
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.volte_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.vt_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.wfc_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += persist.radio.rat_on=combine
PRODUCT_PROPERTY_OVERRIDES += persist.radio.data_ltd_sys_ind=1
PRODUCT_PROPERTY_OVERRIDES += persist.radio.data_con_rprt=1
PRODUCT_PROPERTY_OVERRIDES += persist.radio.calls.on.ims=1

#adb connect
PRODUCT_PROPERTY_OVERRIDES += service.adb.tcp.port=5555
#Two sim card
PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=dsds
PRODUCT_PROPERTY_OVERRIDES += ro.telephony.sim.count=2
PRODUCT_PROPERTY_OVERRIDES += persist.gemini.sim_num=2
PRODUCT_PACKAGES += rild
PRODUCT_PACKAGES += libvlte

#save log to sdcard
PRODUCT_COPY_FILES +=vendor/zebra/zlog.sh:/system/etc/zlog.sh

#ffmpeg
PRODUCT_PACKAGES += libavcodec-57
PRODUCT_PACKAGES += libavdevice-57
PRODUCT_PACKAGES += libavfilter-6
PRODUCT_PACKAGES += libavformat-57
PRODUCT_PACKAGES += libavutil-55
PRODUCT_PACKAGES += libpostproc-54
PRODUCT_PACKAGES += libswresample-2
PRODUCT_PACKAGES += libswscale-4
PRODUCT_PACKAGES += libyuvz

#c10 multi-stream
PRODUCT_PROPERTY_OVERRIDES += persist.radio.mcwill.pid=81.98.97.88
PRODUCT_PROPERTY_OVERRIDES += persist.sys.mag.ip=103.5.126.153
PRODUCT_PROPERTY_OVERRIDES += persist.sys.mag.dns=172.16.251.77
PRODUCT_PROPERTY_OVERRIDES += persist.sys.mag.dns2=8.8.8.8
PRODUCT_PACKAGES += ratd
PRODUCT_PACKAGES += mpc
PRODUCT_PACKAGES += lib-mpd

#add install app
PRODUCT_PACKAGES += mbn_sw.txt
#PRODUCT_PACKAGES += fota
PRODUCT_PACKAGES += webcam
PRODUCT_PACKAGES += mctl
PRODUCT_PACKAGES += vlte
