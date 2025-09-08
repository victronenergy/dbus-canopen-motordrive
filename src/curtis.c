#include <canopen.h>
#include <curtis.h>
#include <localsettings.h>
#include <node.h>
#include <stdlib.h>
#include <string.h>
#include <velib/vecan/products.h>

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Device *device;
    float voltage;

    device = (Device *)request->context;
    voltage = request->response.data * 0.01F;

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
    current = ((sn32)request->response.data) * 0.1F;

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
                   veVariantFloat(&v, request->response.data * 0.1F));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Device *device = (Device *)request->context;
    VeVariant v;
    float torque;

    memcpy(&torque, &request->response.data, sizeof(torque));

    veItemOwnerSet(device->motorTorque, veVariantFloat(&v, fabsf(torque)));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Device *device = (Device *)request->context;
    VeVariant v;

    veItemOwnerSet(device->controllerTemperature,
                   veVariantFloat(&v, request->response.data * 0.1F));
}

static void onError(CanOpenPendingSdoRequest *request) {
    Device *device;

    device = (Device *)request->context;
    disconnectFromNode(device->nodeId);
}

static void readRoutine(Device *device) {
    canOpenReadSdoAsync(device->nodeId, 0x34C1, 0, device,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x338F, 0, device,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x352F, 0, device, onMotorRpmResponse,
                        onError);
    canOpenReadSdoAsync(device->nodeId, 0x3536, 0, device,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x3538, 0, device,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(device->nodeId, 0x3000, 0, device,
                        onControllerTemperatureResponse, onError);
}

Driver curtisDriver = {
    .name = "curtis",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .readRoutine = readRoutine,
};
