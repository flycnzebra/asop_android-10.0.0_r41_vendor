// FIXME: your file license if you have one

#define LOG_TAG "android.hardware.zebra@1.0"
#include <android-base/logging.h>
#include <android-base/file.h>
#include <hidl/HidlTransportSupport.h>

#include "Zebra.h"

namespace android {
namespace hardware {
namespace zebra {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::zebra::V1_0::IZebra follow.
Return<int32_t> Zebra::sendEvent(const hidl_vec<int8_t>& event) {
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        auto ret = (*it)->notifyEvent(event);
        if (!ret.isOk() && ret.isDeadObject()) {
            it = callbacks_.erase(it);
        } else {
            ++it;
        }
    }
    return 0;
}

Return<int32_t> Zebra::registerCallback(const sp<::android::hardware::zebra::V1_0::IZebraCallback>& callback) {
    if (callback == nullptr) {
        return 1;
    }
    {
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        callbacks_.push_back(callback);
    }
    auto linkRet = callback->linkToDeath(this, 0u /* cookie */);
    return 0;
}

Return<int32_t> Zebra::unRegisterCallback(const sp<IZebraCallback>& callback) {
    return unRegisterCallbackInternal(callback) ? 0 : 1;
}

bool Zebra::unRegisterCallbackInternal(const sp<IBase>& callback) {
    if (callback == nullptr) return false;
    bool removed = false;
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        if (interfacesEqual(*it, callback)) {
            it = callbacks_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    (void)callback->unlinkToDeath(this).isOk();  // ignore errors
    return removed;
}

void Zebra::serviceDied(uint64_t /* cookie */, const wp<IBase>& who) {
    (void)unRegisterCallbackInternal(who.promote());
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IZebra* HIDL_FETCH_IZebra(const char* /* name */) {
    return new Zebra();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace zebra
}  // namespace hardware
}  // namespace android
