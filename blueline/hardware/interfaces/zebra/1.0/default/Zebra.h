// FIXME: your file license if you have one

#pragma once

#include <vector>

#include <android/hardware/zebra/1.0/IZebra.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace zebra {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Zebra : public IZebra, hidl_death_recipient {
    // Methods from ::android::hardware::zebra::V1_0::IZebra follow.
    Return<int32_t> sendEvent(const hidl_vec<int8_t>& event) override;
    Return<int32_t> registerCallback(const sp<::android::hardware::zebra::V1_0::IZebraCallback>& callback) override;
    Return<int32_t> unRegisterCallback(const sp<::android::hardware::zebra::V1_0::IZebraCallback>& callback) override;
    void serviceDied(uint64_t cookie, const wp<IBase>& /* who */) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<int32_t> updateAndNotify(const sp<IZebraCallback>& cb);
    bool unRegisterCallbackInternal(const sp<IBase>& cb);
    std::recursive_mutex callbacks_lock_;
    std::vector<sp<IZebraCallback>> callbacks_;
};

// FIXME: most likely delete, this is only for passthrough implementations
extern "C" IZebra* HIDL_FETCH_IZebra(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace zebra
}  // namespace hardware
}  // namespace android
