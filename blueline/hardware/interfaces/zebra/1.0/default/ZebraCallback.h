// FIXME: your file license if you have one

#pragma once

#include <android/hardware/zebra/1.0/IZebraCallback.h>
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

struct ZebraCallback : public IZebraCallback {
    // Methods from ::android::hardware::zebra::V1_0::IZebraCallback follow.
    Return<void> notifyEvent(const hidl_vec<int8_t>& event) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
extern "C" IZebraCallback* HIDL_FETCH_IZebraCallback(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace zebra
}  // namespace hardware
}  // namespace android
