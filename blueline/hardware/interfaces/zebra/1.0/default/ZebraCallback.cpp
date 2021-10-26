// FIXME: your file license if you have one

#include "ZebraCallback.h"

namespace android {
namespace hardware {
namespace zebra {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::zebra::V1_0::IZebraCallback follow.
Return<void> ZebraCallback::notifyEvent(const hidl_vec<int8_t>& event) {
    // TODO implement
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IZebraCallback* HIDL_FETCH_IZebraCallback(const char* /* name */) {
    return new ZebraCallback();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace zebra
}  // namespace hardware
}  // namespace android
