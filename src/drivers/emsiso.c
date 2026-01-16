#include <canopen.h>
#include <drivers/sevcon.h>
#include <localsettings.h>
#include <node.h>
#include <stdlib.h>
#include <string.h>
#include <velib/vecan/products.h>

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float voltage;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&voltage, &request->response.data, sizeof(voltage));

    veItemOwnerSet(node->device->voltage, veVariantFloat(&v, voltage));
    veItemLocalValue(node->device->current, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)(voltage * v.value.Float)));
}

static void onBatteryCurrentResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float current;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&current, &request->response.data, sizeof(current));

    veItemOwnerSet(node->device->current, veVariantFloat(&v, current));

    veItemLocalValue(node->device->voltage, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)(v.value.Float * current)));
}

static void onMotorRpmResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float fRpm;
    sn16 rpm;
    un8 motorDirection;
    veBool motorDirectionInverted;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&fRpm, &request->response.data, sizeof(fRpm));
    rpm = (sn16)fRpm;

    veItemOwnerSet(node->device->motorRpm, veVariantUn16(&v, abs(rpm)));

    veItemLocalValue(node->device->motorDirectionInverted, &v);
    motorDirectionInverted = v.value.SN32 == 1;
    // 0 - neutral, 1 - reverse, 2 - forward
    if (rpm > 0) {
        motorDirection = motorDirectionInverted ? 1 : 2;
    } else if (rpm < 0) {
        motorDirection = motorDirectionInverted ? 2 : 1;
    } else {
        motorDirection = 0;
    }
    veItemOwnerSet(node->device->motorDirection,
                   veVariantUn8(&v, motorDirection));
}

static void onMotorTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;
    float temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&temperature, &request->response.data, sizeof(temperature));

    veItemOwnerSet(node->device->motorTemperature,
                   veVariantFloat(&v, temperature));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;
    float torque;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&torque, &request->response.data, sizeof(torque));

    veItemOwnerSet(node->device->motorTorque, veVariantFloat(&v, torque));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;
    float temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&temperature, &request->response.data, sizeof(temperature));

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantFloat(&v, temperature));
}

static void onCoolantTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;
    float temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    memcpy(&temperature, &request->response.data, sizeof(temperature));

    veItemOwnerSet(node->device->coolantTemperature,
                   veVariantFloat(&v, temperature));
}

static void onError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    disconnectFromNode(node->device->nodeId);
}

static void readRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x31f0, 0x08, node,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x31f0, 0x07, node,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3101, 0x01, node,
                        onMotorRpmResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3101, 0x0b, node,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3101, 0x03, node,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x31cd, 0x00, node,
                        onControllerTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x31cf, 0x00, node,
                        onCoolantTemperatureResponse, onError);
}

static void fastReadRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x3101, 0x01, node,
                        onMotorRpmResponse, onError);
}

static un8 name[255];
static un8 length;

static void onSerialNumberResponse(CanOpenPendingSdoRequest *request) {
    FetchSerialNumberRequest *fetchRequest;

    fetchRequest = (FetchSerialNumberRequest *)request->context;
    name[length] = '\0';
    fetchRequest->onSuccess(fetchRequest, (char *)name);
}

static void onSerialNumberError(CanOpenPendingSdoRequest *request,
                                CanOpenError error) {
    FetchSerialNumberRequest *fetchRequest;

    fetchRequest = (FetchSerialNumberRequest *)request->context;
    fetchRequest->onError(fetchRequest);
}

static void fetchSerialNumber(FetchSerialNumberRequest *request) {
    canOpenReadSegmentedSdoAsync(request->nodeId, 0x2007, 0x01, request, name,
                                 &length, sizeof(name) - 1,
                                 onSerialNumberResponse, onSerialNumberError);
}

Driver emsisoDriver = {
    .name = "emsiso",
    .productId = VE_PROD_ID_EPROPULSION,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = NULL,
    .destroyDriverContext = NULL,
    .onEMCYMessage = NULL,
    .fetchSerialNumber = fetchSerialNumber,
};