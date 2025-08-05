#include <canopen.h>
#include <localsettings.h>
#include <node.h>
#include <sevcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <velib/vecan/products.h>

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Device *device;
    float voltage;

    device = (Device *)request->context;
    voltage = request->response.data * 0.0625F;

    veItemOwnerSet(device->voltage, veVariantFloat(&v, voltage));
    veItemLocalValue(device->current, &v);
    veItemOwnerSet(device->power,
                   veVariantSn32(&v, (sn32)voltage * v.value.Float));
}

static void onBatteryCurrentResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Device *device;
    float current;

    device = (Device *)request->context;
    current = ((sn32)request->response.data) * 0.0625F;

    veItemOwnerSet(device->current, veVariantFloat(&v, current));

    veItemLocalValue(device->voltage, &v);
    veItemOwnerSet(device->power,
                   veVariantSn32(&v, (sn32)v.value.Float * current));
}

static void onMotorRpmResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Device *device;
    sn16 rpm;
    un8 motorDirection;
    veBool motorDirectionInverted;

    device = (Device *)request->context;
    rpm = request->response.data;

    veItemOwnerSet(device->motorRpm, veVariantUn16(&v, abs(rpm)));

    veItemLocalValue(device->motorDirectionInverted, &v);
    motorDirectionInverted = veVariantIsValid(&v) && v.value.SN32 == 1;
    // 0 - neutral, 1 - reverse, 2 - forward
    if (rpm > 0) {
        motorDirection = motorDirectionInverted ? 1 : 2;
    } else if (rpm < 0) {
        motorDirection = motorDirectionInverted ? 2 : 1;
    } else {
        motorDirection = 0;
    }
    veItemOwnerSet(device->motorDirection, veVariantUn8(&v, motorDirection));
}

static void onMotorTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Device *device = (Device *)request->context;
    VeVariant v;

    veItemOwnerSet(device->motorTemperature,
                   veVariantUn16(&v, request->response.data));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Device *device = (Device *)request->context;
    VeVariant v;

    veItemOwnerSet(device->motorTorque,
                   veVariantUn16(&v, request->response.data));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Device *device = (Device *)request->context;
    VeVariant v;

    veItemOwnerSet(device->controllerTemperature,
                   veVariantUn16(&v, request->response.data));
}

static void onError(CanOpenPendingSdoRequest *request) {
    Device *device;

    device = (Device *)request->context;
    disconnectFromNode(device->nodeId);
}

static void readRoutine(Device *device) {
    canOpenReadSdoAsync(device->nodeId, 0x5100, 1, device,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x5100, 2, device,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x606c, 0, device, onMotorRpmResponse,
                        onError);
    canOpenReadSdoAsync(device->nodeId, 0x4600, 3, device,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x4602, 0xC, device,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x5100, 4, device,
                        onControllerTemperatureResponse, onError);
}

Driver sevconDriver = {
    .name = "sevcon",
    .productId = VE_PROD_ID_SEVCON_MOTORDRIVE,
    .readRoutine = readRoutine,
};