#PRODUCT_PACKAGES += FlyOta

PRODUCT_COPY_FILES += vendor/copyfiles/mbn_sw.txt:$(TARGET_COPY_OUT_VENDOR)/rfs/msm/mpss/readonly/vendor/mbn/mcfg_sw/mbn_sw.txt

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
