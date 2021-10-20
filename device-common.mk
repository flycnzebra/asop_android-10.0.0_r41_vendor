#support china mobile sim
PRODUCT_PROPERTY_OVERRIDES += \
ro.mtk_ims_support=1
PRODUCT_PROPERTY_OVERRIDES += \
ro.mtk_volte_support=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.mtk.volte.enable=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.dbg.volte_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.dbg.ims_volte_enable=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.dbg.volte_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.dbg.vt_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.dbg.wfc_avail_ovr=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.radio.rat_on=combine
PRODUCT_PROPERTY_OVERRIDES += \
persist.radio.data_ltd_sys_ind=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.radio.data_con_rprt=1
PRODUCT_PROPERTY_OVERRIDES += \
persist.radio.calls.on.ims=1

#adb connect
PRODUCT_PROPERTY_OVERRIDES += \
service.adb.tcp.port=5555

#add install app
PRODUCT_PACKAGES += mbn_sw.txt
#PRODUCT_PACKAGES += fota
PRODUCT_PACKAGES += webcam
PRODUCT_PACKAGES += mctl
PRODUCT_PACKAGES += vlte
PRODUCT_PACKAGES += libvlte

##c10 m-stream
#PRODUCT_PROPERTY_OVERRIDES += \
#persist.radio.mcwill.pid=81.98.97.88
#PRODUCT_PACKAGES += ratd
#PRODUCT_PACKAGES += mpc
#PRODUCT_PACKAGES += lib-mpd