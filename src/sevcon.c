#include <canopen.h>
#include <localsettings.h>
#include <logger.h>
#include <sevcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <velib/platform/plt.h>

static struct VeSettingProperties booleanType = {
    .type = VE_SN32,
    .def.value.SN32 = 0,
    .min.value.SN32 = 0,
    .max.value.SN32 = 1,
};

static void onBeforeDbusInit(Device *device) {
    char settingsPath[128];
    SevconDriverContext *context;

    context = malloc(sizeof(SevconDriverContext));
    if (context == NULL) {
        error("could not allocate SevconDriverContext");
        pltExit(5);
    }

    snprintf(settingsPath, sizeof(settingsPath), "Settings/Devices/%s",
             device->identifier);
    context->directionInverted =
        veItemCreateSettingsProxy(localSettings, settingsPath, device->root,
                                  "Settings/Motor/DirectionInverted",
                                  veVariantFmt, &veUnitNone, &booleanType);
    device->driverContext = context;
}

static void onDestroy(Device *device) { free(device->driverContext); }

static veBool readBatteryVoltage(un8 nodeId, float *voltage) {
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 1, &response) != 0) {
        return veTrue;
    }
    *voltage = response.data * 0.0625F;
    return veFalse;
}

static veBool readBatteryCurrent(un8 nodeId, float *current) {
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 2, &response) != 0) {
        return veTrue;
    }
    *current = response.data * 0.0625F;
    return veFalse;
}

static veBool readEngineRpm(un8 nodeId, sn16 *rpm) {
    SdoMessage response;
    if (readSdo(nodeId, 0x606c, 0, &response) != 0) {
        return veTrue;
    }
    *rpm = response.data;
    return veFalse;
}

static veBool readEngineTemperature(un8 nodeId, un16 *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x4600, 3, &response) != 0) {
        return veTrue;
    }
    *temperature = response.data;
    return veFalse;
}

static veBool getSerialNumber(un8 nodeId, un32 *serialNumber) {
    SdoMessage response;
    if (readSdo(nodeId, 0x1018, 4, &response) != 0) {
        return veTrue;
    }
    *serialNumber = response.data;
    return veFalse;
}

static veBool readRoutine(Device *device) {
    float batteryVoltage;
    float batteryCurrent;
    sn16 engineRpm;
    un16 engineTemperature;
    un8 engineDirection;
    veBool directionInverted;
    VeVariant v;

    if (readBatteryVoltage(device->nodeId, &batteryVoltage) ||
        readBatteryCurrent(device->nodeId, &batteryCurrent) ||
        readEngineRpm(device->nodeId, &engineRpm) ||
        readEngineTemperature(device->nodeId, &engineTemperature)) {
        return veTrue;
    }

    veItemLocalValue(
        ((SevconDriverContext *)device->driverContext)->directionInverted, &v);
    directionInverted = veVariantIsValid(&v) && v.value.SN32 == 1;
    // 0 - neutral, 1 - reverse, 2 - forward
    if (engineRpm > 0) {
        engineDirection = directionInverted ? 1 : 2;
    } else if (engineRpm < 0) {
        engineDirection = directionInverted ? 2 : 1;
    } else {
        engineDirection = 0;
    }

    veItemOwnerSet(device->voltage, veVariantFloat(&v, batteryVoltage));
    veItemOwnerSet(device->current, veVariantFloat(&v, batteryCurrent));
    veItemOwnerSet(device->rpm, veVariantUn16(&v, abs(engineRpm)));
    veItemOwnerSet(device->temperature, veVariantUn16(&v, engineTemperature));
    veItemOwnerSet(device->direction, veVariantUn8(&v, engineDirection));

    return veFalse;
}

Driver sevconDriver = {
    .name = "sevcon",
    .getSerialNumber = getSerialNumber,
    .readRoutine = readRoutine,
    .onBeforeDbusInit = onBeforeDbusInit,
    .onDestroy = onDestroy,
};