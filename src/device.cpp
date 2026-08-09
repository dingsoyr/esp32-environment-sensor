#include "device.h"

String getDeviceId() {
    uint64_t chipId = ESP.getEfuseMac();

    char deviceId[20];

    snprintf(
        deviceId,
        sizeof(deviceId),
        "sensor-%06llx",
        chipId & 0xFFFFFF
    );

    return String(deviceId);
}