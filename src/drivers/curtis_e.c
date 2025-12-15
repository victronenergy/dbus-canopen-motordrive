#include <canopen.h>
#include <drivers/curtis_e.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>
#include <velib/vecan/products.h>

#define RPM_DEADBAND 3
#define CURRENT_DEADBAND 0.1F

static void onSwapMotorDirectionResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    CurtisEContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = (CurtisEContext *)node->device->driverContext;
    context->swapMotorDirection = request->response.data & 0b1000;
}

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float voltage;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    voltage = ((sn16)request->response.data) * 0.015625F;

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

    current = ((sn16)request->response.data) * 0.1F;
    if (current >= -CURRENT_DEADBAND && current <= CURRENT_DEADBAND) {
        current = 0.0F;
    }

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
    CurtisEContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = (CurtisEContext *)node->device->driverContext;

    rpm = request->response.data;
    if (context->swapMotorDirection == 1) { // Throttle is reversed
        rpm *= -1;
    }
    if (rpm > -RPM_DEADBAND && rpm < RPM_DEADBAND) {
        rpm = 0;
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
                   veVariantFloat(&v, ((sn16)request->response.data) * 0.1F));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantFloat(&v, ((sn16)request->response.data) * 0.1F));
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
    CurtisEContext *context;

    context = (CurtisEContext *)node->device->driverContext;
    if (context->swapMotorDirection == -1) {
        canOpenReadSdoAsync(node->device->nodeId, 0x306C, 0, node,
                            onSwapMotorDirectionResponse, onError);
    }
    canOpenReadSdoAsync(node->device->nodeId, 0x324C, 0, node,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x359E, 0, node,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3207, 0, node,
                        onMotorRpmResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x320B, 0, node,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x322A, 0, node,
                        onControllerTemperatureResponse, onError);
}

static void fastReadRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x3207, 0, node,
                        onMotorRpmResponse, onError);
}

static void *createDriverContext(Node *node) {
    CurtisEContext *context;

    context = malloc(sizeof(*context));
    if (!context) {
        error("malloc failed for CurtisEContext");
        pltExit(5);
    }

    context->swapMotorDirection = -1;

    return (void *)context;
}

static void destroyDriverContext(Node *node, void *context) { free(context); }

Driver curtisEDriver = {
    .name = "curtis_e",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = createDriverContext,
    .destroyDriverContext = destroyDriverContext,
};
