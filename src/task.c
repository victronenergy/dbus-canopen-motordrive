#include <canopen.h>
#include <ctype.h>
#include <device.h>
#include <sevcon.h>
#include <stdio.h>
#include <string.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/platform/plt.h>
#include <velib/utils/ve_logger.h>
#include <velib/utils/ve_timer.h>

static un8 nodeId = 1;
static un16 task1sLastUpdate = 0;
static un16 task10sLastUpdate = 0;
static Device device;
static veBool connected = veFalse;
static veBool directionFlipped = veFalse;

void taskEarlyInit(void)
{
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void taskInit(void) {}

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
    un32 engineRpm;
    un16 engineTemperature;
    un8 engineDirection;
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

    if (engineRpm > 0) {
        engineDirection = directionFlipped ? 1 : 2;
    } else if (engineRpm < 0) {
        engineDirection = directionFlipped ? 2 : 1;
    } else {
        engineDirection = 0;
    }

    veItemOwnerSet(&device.voltage, veVariantFloat(&v, batteryVoltage));
    veItemOwnerSet(&device.current, veVariantFloat(&v, batteryCurrent));
    veItemOwnerSet(&device.rpm, veVariantUn16(&v, engineRpm));
    veItemOwnerSet(&device.temperature, veVariantUn16(&v, engineTemperature));
    veItemOwnerSet(&device.direction, veVariantUn8(&v, engineDirection));
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
    if (veTick1ms(&task1sLastUpdate, 1000)) {
        task1s();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

void taskTick(void) {}

char const *pltProgramVersion(void) { return "v0.0"; }