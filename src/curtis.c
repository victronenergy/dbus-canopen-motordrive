#include <canopen.h>
#include <curtis.h>
#include <localsettings.h>
#include <stdlib.h>
#include <string.h>
#include <velib/vecan/products.h>

static void onBeforeDbusInit(Device *device) {
    // Add any dbus item specific to curtis here
}

static void onDestroy(Device *device) {
    // Cleanup any allocated resources specific to curtis here
}

static veBool getSerialNumber(un8 nodeId, un32 *serialNumber) {
    SdoMessage response;
    if (readSdo(nodeId, 0x1018, 4, &response) != 0) {
        return veTrue;
    }
    *serialNumber = response.data;
    return veFalse;
}

static veBool readBatteryVoltage(un8 nodeId, float *voltage) {
    SdoMessage response;
    if (readSdo(nodeId, 0x34C1, 0, &response) != 0) {
        return veTrue;
    }
    *voltage = response.data * 0.01F;

    return veFalse;
}

static veBool readBatteryCurrent(un8 nodeId, float *current) {
    SdoMessage response;
    if (readSdo(nodeId, 0x338F, 0, &response) != 0) {
        return veTrue;
    }
    *current = response.data * 0.1F;
    return veFalse;
}

static veBool readMotorRpm(un8 nodeId, sn16 *rpm) {
    SdoMessage response;
    if (readSdo(nodeId, 0x352F, 0, &response) != 0) {
        return veTrue;
    }
    *rpm = response.data;

    SdoMessage revResponse;
    if (readSdo(nodeId, 0x362F, 0, &revResponse) == 0) {
        if (revResponse.data == 1) {
            *rpm *= -1;
        }
    }
    return veFalse;
}

static veBool readMotorTorque(un8 nodeId, float *torque) {
    SdoMessage response;
    if (readSdo(nodeId, 0x3538, 0, &response) != 0) {
        return veTrue;
    }
    memcpy(torque, &response.data,
           sizeof(*torque)); // Torque is sent as 32bit float
    return veFalse;
}

static veBool readMotorTemperature(un8 nodeId, float *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x3536, 0, &response) != 0) {
        return veTrue;
    }
    *temperature = response.data * 0.1F;
    return veFalse;
}

static veBool readControllerTemperature(un8 nodeId, float *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x3000, 0, &response) != 0) {
        return veTrue;
    }
    *temperature = response.data * 0.1F;
    return veFalse;
}

static veBool readRoutine(Device *device) {
    float batteryVoltage;
    float batteryCurrent;
    sn16 motorRpm;
    float motorTemperature;
    float motorTorque;
    un8 motorDirection;
    veBool motorDirectionInverted;
    float controllerTemperature;
    VeVariant v;

    if (readBatteryVoltage(device->nodeId, &batteryVoltage) ||
        readBatteryCurrent(device->nodeId, &batteryCurrent) ||
        readMotorRpm(device->nodeId, &motorRpm) ||
        readMotorTorque(device->nodeId, &motorTorque) ||
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
    veItemOwnerSet(device->power,
                   veVariantSn32(&v, (sn32)batteryVoltage * batteryCurrent));
    veItemOwnerSet(device->motorRpm, veVariantUn16(&v, abs(motorRpm)));
    veItemOwnerSet(device->motorTemperature,
                   veVariantFloat(&v, motorTemperature));
    veItemOwnerSet(device->motorTorque, veVariantFloat(&v, fabs(motorTorque)));
    veItemOwnerSet(device->controllerTemperature,
                   veVariantFloat(&v, controllerTemperature));
    veItemOwnerSet(device->motorDirection, veVariantUn8(&v, motorDirection));

    return veFalse;
}

Driver curtisDriver = {
    .name = "curtis",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .getSerialNumber = getSerialNumber,
    .readRoutine = readRoutine,
    .onBeforeDbusInit = onBeforeDbusInit,
    .onDestroy = onDestroy,
};
