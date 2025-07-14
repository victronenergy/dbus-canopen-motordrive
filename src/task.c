#include <device.h>
#include <localsettings.h>
#include <sevcon.h>
#include <stdlib.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

static un8 nodeId = 1;
static un16 task1sLastUpdate = 0;
static un16 task10sLastUpdate = 0;
static Device device;
static veBool connected = veFalse;

void taskEarlyInit(void) {
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void taskInit(void) {
    localSettingsInit();

    // @todo: get global settings for nodeId
}

void connectDevice() {
    un32 serialNumber;

    if (sevconFetchSerialNumber(nodeId, &serialNumber)) {
        return;
    }

    createDevice(&device, serialNumber);
    connected = veTrue;
}

void disconnectDevice() {
    destroyDevice(&device);
    connected = veFalse;
}

void task1s() {
    float batteryVoltage;
    float batteryCurrent;
    sn16 engineRpm;
    un16 engineTemperature;
    un8 engineDirection;
    veBool directionInverted;
    VeVariant v;

    if (!connected) {
        return;
    }

    if (sevconFetchBatteryVoltage(nodeId, &batteryVoltage)) {
        disconnectDevice();
        return;
    }
    if (sevconFetchBatteryCurrent(nodeId, &batteryCurrent)) {
        disconnectDevice();
        return;
    }
    if (sevconFetchEngineRpm(nodeId, &engineRpm)) {
        disconnectDevice();
        return;
    }
    if (sevconFetchEngineTemperature(nodeId, &engineTemperature)) {
        disconnectDevice();
        return;
    }

    veItemLocalValue(device.directionInverted, &v);
    directionInverted = veVariantIsValid(&v) && v.value.SN32 == 1;
    // 0 - neutral, 1 - reverse, 2 - forward
    if (engineRpm > 0) {
        engineDirection = directionInverted ? 1 : 2;
    } else if (engineRpm < 0) {
        engineDirection = directionInverted ? 2 : 1;
    } else {
        engineDirection = 0;
    }

    veItemOwnerSet(device.voltage, veVariantFloat(&v, batteryVoltage));
    veItemOwnerSet(device.current, veVariantFloat(&v, batteryCurrent));
    veItemOwnerSet(device.rpm, veVariantUn16(&v, abs(engineRpm)));
    veItemOwnerSet(device.temperature, veVariantUn16(&v, engineTemperature));
    veItemOwnerSet(device.direction, veVariantUn8(&v, engineDirection));
}

void task10s() {
    if (connected) {
        return;
    }

    if (sevconLogin(nodeId)) {
        return;
    }

    connectDevice();
}

void taskUpdate(void) {
    VeRawCanMsg msg;

    while (veCanRead(&msg)) {
        // Prevents high CPU usage if nothing else consumes the CAN messages
    }

    if (veTick1ms(&task1sLastUpdate, 1000)) {
        task1s();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

void taskTick(void) {
    if (device.root) {
        veItemTick(device.root);
    }
}

char const *pltProgramVersion(void) { return VERSION; }