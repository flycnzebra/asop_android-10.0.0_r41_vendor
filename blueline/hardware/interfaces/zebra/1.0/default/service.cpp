#define LOG_TAG "android.hardware.zebra@1.0-service"

#include <android/hardware/zebra/1.0/IZebra.h>
#include <hidl/LegacySupport.h>

using android::hardware::zebra::V1_0::IZebra;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    ALOGE("zebra service is running...");
    return defaultPassthroughServiceImplementation<IZebra>(2);
}
