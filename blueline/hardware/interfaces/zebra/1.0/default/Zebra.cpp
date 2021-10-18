// FIXME: your file license if you have one

#include "Zebra.h"

namespace android {
namespace hardware {
namespace zebra {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::zebra::V1_0::IZebra follow.
Return<void> Zebra::helloWorld(const hidl_string& name, helloWorld_cb _hidl_cb) {
    char buf[100];
    ::memset(buf, 0x00,100);
    ::snprintf(buf,100,"Hello World, %s", name.c_str());
    hidl_string result(buf);
    _hidl_cb(result);
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IZebra* HIDL_FETCH_IZebra(const char* /* name */) {
    return new Zebra();
}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace zebra
}  // namespace hardware
}  // namespace android
