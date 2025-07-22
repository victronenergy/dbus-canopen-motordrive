#include <canopen.h>
#include <localsettings.h>
#include <sevcon.h>
#include <stdlib.h>

static void onBeforeDbusInit(Device *device) {}

static void onDestroy(Device *device) {}

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

static veBool readMotorRpm(un8 nodeId, sn16 *rpm) {
    SdoMessage response;
    if (readSdo(nodeId, 0x606c, 0, &response) != 0) {
        return veTrue;
    }
    *rpm = response.data;
    return veFalse;
}

static veBool readMotorTemperature(un8 nodeId, un16 *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x4600, 3, &response) != 0) {
        return veTrue;
    }
    *temperature = response.data;
    return veFalse;
}

static veBool readControllerTemperature(un8 nodeId, un16 *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 4, &response) != 0) {
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
    sn16 motorRpm;
    un16 motorTemperature;
    un8 motorDirection;
    veBool motorDirectionInverted;
    un16 controllerTemperature;
    VeVariant v;

    if (readBatteryVoltage(device->nodeId, &batteryVoltage) ||
        readBatteryCurrent(device->nodeId, &batteryCurrent) ||
        readMotorRpm(device->nodeId, &motorRpm) ||
        readMotorTemperature(device->nodeId, &motorTemperature) ||
        readControllerTemperature(device->nodeId, &controllerTemperature)) {
        return veTrue;
    }

    veItemLocalValue(device->motorDirectionInverted, &v);
    motorDirectionInverted = veVariantIsValid(&v) && v.value.SN32 == 1;
    // 0 - neutral, 1 - reverse, 2 - forward
    if (motorRpm > 0) {
        motorDirection = motorDirectionInverted ? 1 : 2;
    } else if (motorRpm < 0) {
        motorDirection = motorDirectionInverted ? 2 : 1;
    } else {
        motorDirection = 0;
    }

    veItemOwnerSet(device->voltage, veVariantFloat(&v, batteryVoltage));
    veItemOwnerSet(device->current, veVariantFloat(&v, batteryCurrent));
    veItemOwnerSet(device->motorRpm, veVariantUn16(&v, abs(motorRpm)));
    veItemOwnerSet(device->motorTemperature,
                   veVariantUn16(&v, motorTemperature));
    veItemOwnerSet(device->motorDirection, veVariantUn8(&v, motorDirection));
    veItemOwnerSet(device->controllerTemperature,
                   veVariantUn16(&v, controllerTemperature));

    return veFalse;
}

Driver sevconDriver = {
    .name = "sevcon",
    .getSerialNumber = getSerialNumber,
    .readRoutine = readRoutine,
    .onBeforeDbusInit = onBeforeDbusInit,
    .onDestroy = onDestroy,
};