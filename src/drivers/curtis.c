#include <canopen.h>
#include <drivers/curtis.h>
#include <localsettings.h>
#include <node.h>
#include <stdlib.h>
#include <string.h>
#include <velib/vecan/products.h>

// @todo: this isn't going to work with more than one node using this driver
static int swapMotorDirection = -1; // cache for Swap_Motor_Direction

static void onSwapMotorDirectionResponse(CanOpenPendingSdoRequest *request) {
    swapMotorDirection = request->response.data;
}

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float voltage;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    voltage = request->response.data * 0.01F;

    veItemOwnerSet(node->device->voltage, veVariantFloat(&v, voltage));
    veItemLocalValue(node->device->current, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)voltage * v.value.Float));
}

static void onBatteryCurrentResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float current;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    current = ((sn32)request->response.data) * 0.1F;

    veItemOwnerSet(node->device->current, veVariantFloat(&v, current));

    veItemLocalValue(node->device->voltage, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)v.value.Float * current));
}

static void onMotorRpmResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    sn16 rpm;
    un8 motorDirection;
    veBool motorDirectionInverted;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    rpm = request->response.data;
    if (swapMotorDirection == 1) { // Throttle is reversed
        rpm *= -1;
    }

    veItemOwnerSet(node->device->motorRpm, veVariantUn16(&v, abs(rpm)));

    veItemLocalValue(node->device->motorDirectionInverted, &v);
    motorDirectionInverted = veVariantIsValid(&v) && v.value.SN32 == 1;
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

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->motorTemperature,
                   veVariantFloat(&v, request->response.data * 0.1F));
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

    veItemOwnerSet(node->device->motorTorque,
                   veVariantFloat(&v, fabsf(torque)));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantFloat(&v, request->response.data * 0.1F));
}

static void onError(CanOpenPendingSdoRequest *request) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    disconnectFromNode(node->device->nodeId);
}

static void readRoutine(Node *node) {
    if (swapMotorDirection == -1) {
        canOpenReadSdoAsync(node->device->nodeId, 0x362F, 0, node,
                            onSwapMotorDirectionResponse, onError);
    }
    canOpenReadSdoAsync(node->device->nodeId, 0x34C1, 0, node,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x338F, 0, node,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x352F, 0, node,
                        onMotorRpmResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3536, 0, node,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3538, 0, node,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3000, 0, node,
                        onControllerTemperatureResponse, onError);
}

Driver curtisDriver = {
    .name = "curtis",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .readRoutine = readRoutine,
};
