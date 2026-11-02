#include <canopen.h>
#include <drivers/dmc.h>
#include <logger.h>
#include <math.h>
#include <memory.h>
#include <node.h>
#include <notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_values.h>
#include <velib/vecan/products.h>

// Temperatures are reported as an unsigned byte with a fixed offset.
#define TEMPERATURE_OFFSET 51

#define GET_DMC_CONTEXT(a) ((DmcContext *)(a->device->driverContext))

typedef struct {
    un8 faultCode;
    char const *error;
} Error;

// Base fault codes, see Sigma2N Traction Advanced Manual V1.11 section 8.1.
// The sub fault code is reported alongside but not decoded, as its meaning
// depends on the base fault code and on the motor module in use.
static Error errorDb[] = {{2, "Voltage getting low"},
                          {3, "Inhibit drive / BDI cut / BCL via CAN"},
                          {4, "Voltage getting high"},
                          {5, "Motor temperature high"},
                          {6, "Controller temperature high"},
                          {7, "Adjustment out of range"},
                          {8, "Default adjustments used"},
                          {9, "Memory chip fault"},
                          {10, "Both forward and reverse inputs active"},
                          {11, "Drive not allowed"},
                          {12, "Power up sequence fault"},
                          {13, "Accelerator more than 50% at power up"},
                          {14, "Bellyswitch and inching fault"},
                          {15, "Supply voltage fault"},
                          {16, "Dual motor soft error"},
                          {17, "Battery voltage too low"},
                          {18, "High sided mosfets short circuit"},
                          {19, "Motor stall protection"},
                          {20, "Hardware over current detected"},
                          {21, "Contactor coil driver fault"},
                          {22, "Battery voltage too high"},
                          {23, "Low sided mosfets short circuit in neutral"},
                          {24, "Hardware fail safe fault"},
                          {25, "Line contactor fault"},
                          {26, "Thermal shutdown fault"},
                          {27, "Low sided mosfets short circuit at power up"},
                          {28, "Wire off detected"},
                          {29, "CAN node fault"},
                          {30, "Motor overspeeding"},
                          {31, "Motor fault"},
                          {32, "Motor module initialization error"},
                          {33, "Motor module configuration inconsistency"},
                          {34, "Motor parameter inconsistency"},
                          {35, "Current sensor calibration fault"},
                          {36, "Controller temperature over 100 degrees"},
                          {39, "Generic time out"},
                          {40, "System fault"}};

static Error *findError(un8 faultCode) {
    size_t dbSize = sizeof(errorDb) / sizeof(Error);
    for (size_t i = 0; i < dbSize; i++) {
        if (errorDb[i].faultCode == faultCode) {
            return &errorDb[i];
        }
    }
    return NULL;
}

static sn16 higherOf(sn16 a, sn16 b) { return a > b ? a : b; }

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float voltage;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    voltage = ((sn16)request->response.data) * 0.1F;

    veItemOwnerSet(node->device->voltage, veVariantFloat(&v, voltage));
}

static void onBatteryCurrentResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;
    float current;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    current = ((sn16)request->response.data) * 0.1F;
    context = GET_DMC_CONTEXT(node);
    context->master.current = current;
}

static void onMotorRpmResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    sn32 rpm;
    un8 motorDirection;
    veBool motorDirectionInverted;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    rpm = (sn32)request->response.data;

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
    DmcContext *context;
    sn16 temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    temperature = ((un8)request->response.data) - TEMPERATURE_OFFSET;
    context = GET_DMC_CONTEXT(node);
    context->master.motorTemperature = temperature;
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;
    float torque;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    torque = ((sn16)request->response.data) * 0.1F;
    context = GET_DMC_CONTEXT(node);
    context->master.torque = torque;
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;
    sn16 temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    temperature = ((un8)request->response.data) - TEMPERATURE_OFFSET;
    context = GET_DMC_CONTEXT(node);
    context->master.controllerTemperature = temperature;
}

static void onError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    disconnectFromNode(node->device->nodeId);
}

// A fault on the slave is reported on the master's device, which is the only
// one the user sees, and names the slave so it can be found on the bus.
static void reportFault(Node *node, un8 slaveNodeId, un8 faultCode,
                        un32 faultSubcode) {
    Error *error;
    char notificationTitle[255];
    char notificationDescription[255];
    VeStr deviceName;

    error = findError(faultCode);
    if (error != NULL) {
        snprintf(notificationTitle, sizeof(notificationTitle), "%s (F%d S%u)",
                 error->error, faultCode, faultSubcode);
    } else {
        snprintf(notificationTitle, sizeof(notificationTitle),
                 "Unknown fault (F%d S%u)", faultCode, faultSubcode);
    }

    getDeviceDisplayName(node->device, &deviceName);
    if (slaveNodeId != 0) {
        error("EMCY from node %d, slave of node %d: %s", slaveNodeId,
              node->device->nodeId, notificationTitle);
        snprintf(notificationDescription, sizeof(notificationDescription),
                 "%s, slave node %u", veStrCStr(&deviceName), slaveNodeId);
    } else {
        error("EMCY from node %d: %s", node->device->nodeId, notificationTitle);
        snprintf(notificationDescription, sizeof(notificationDescription), "%s",
                 veStrCStr(&deviceName));
    }
    queueNotification(node->device->nodeId, NOTIFICATION_TYPE_ERROR,
                      notificationTitle, notificationDescription);
    veStrFree(&deviceName);
}

// 0x3840 is read immediately before 0x3841 and the request queue is served in
// order, so the code belonging to this subcode is already in the fault state.
static void handleFaultSubcode(Node *node, DmcFaultState *state,
                               un8 slaveNodeId, un16 faultSubcode) {
    un8 faultCode;

    faultCode = state->pendingCode;
    if (faultCode == state->reportedCode &&
        faultSubcode == state->reportedSubcode) {
        return;
    }

    state->reportedCode = faultCode;
    state->reportedSubcode = faultSubcode;

    if (faultCode == 0) {
        // Fault cleared.
        return;
    }

    reportFault(node, slaveNodeId, faultCode, faultSubcode);
}

static void onFaultCodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->fault.pendingCode = (un8)request->response.data;
}

static void onFaultSubcodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    handleFaultSubcode(node, &context->fault, 0, (un16)request->response.data);
}

static void onSlaveCurrentResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slave.current = ((sn16)request->response.data) * 0.1F;
}

static void onSlaveTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slave.torque = ((sn16)request->response.data) * 0.1F;
}

static void onSlaveMotorTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slave.motorTemperature =
        ((un8)request->response.data) - TEMPERATURE_OFFSET;
}

static void
onSlaveControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slave.controllerTemperature =
        ((un8)request->response.data) - TEMPERATURE_OFFSET;
}

static void onSlaveFaultCodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slaveFault.pendingCode = (un8)request->response.data;
}

// The slave's last read of the cycle, so everything is in.
static void onSlaveFaultSubcodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    if (!context->slaveConnected) {
        // A slave read of this cycle failed, maybe 0x3840, whose code would
        // then be an earlier cycle's. The fault is read again next cycle.
        return;
    }
    handleFaultSubcode(node, &context->slaveFault, request->nodeId,
                       (un16)request->response.data);
}

static void onSlaveError(CanOpenPendingSdoRequest *request,
                         CanOpenError error) {
    Node *node;
    DmcContext *context;
    VeStr deviceName;
    char description[255];

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);

    if (context->slaveConnected && error == SDO_READ_ERROR_TIMEOUT) {
        warning("Slave node %u of node %u is not responding", request->nodeId,
                node->device->nodeId);
        getDeviceDisplayName(node->device, &deviceName);
        snprintf(description, sizeof(description), "%s, slave node %u",
                 veStrCStr(&deviceName), request->nodeId);
        queueNotification(node->device->nodeId, NOTIFICATION_TYPE_WARNING,
                          "Slave controller not responding", description);
        veStrFree(&deviceName);
    }

    context->slaveConnected = veFalse;
}

static void onSlaveProbeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->slaveConnected = veTrue;
    info("Slave node %u of node %u is responding again", request->nodeId,
         node->device->nodeId);
}

static void onSlaveProbeError(CanOpenPendingSdoRequest *request,
                              CanOpenError error) {}

static void publishSingleDrive(Node *node, DmcContext *context) {
    VeVariant v;
    VeVariant voltage;
    VeVariant current;

    veItemOwnerSet(node->device->current,
                   veVariantFloat(&v, context->master.current));
    veItemOwnerSet(node->device->motorTorque,
                   veVariantFloat(&v, fabsf(context->master.torque)));

    veItemLocalValue(node->device->voltage, &voltage);
    veItemLocalValue(node->device->current, &current);
    veItemOwnerSet(
        node->device->power,
        veVariantSn32(&v, (sn32)(voltage.value.Float * current.value.Float)));

    veItemOwnerSet(node->device->motorTemperature,
                   veVariantSn16(&v, context->master.motorTemperature));
    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantSn16(&v, context->master.controllerTemperature));
}

static void publishDualDrive(Node *node, DmcContext *context) {
    VeVariant v;
    VeVariant voltage;
    VeVariant current;

    if (!context->slaveProbing && context->slaveConnected) {
        veItemOwnerSet(node->device->current,
                       veVariantFloat(&v, context->master.current +
                                              context->slave.current));
        veItemLocalValue(node->device->voltage, &voltage);
        veItemLocalValue(node->device->current, &current);
        veItemOwnerSet(node->device->power,
                       veVariantSn32(&v, (sn32)(voltage.value.Float *
                                                current.value.Float)));

        veItemOwnerSet(node->device->motorTorque,
                       veVariantFloat(&v, fabsf(context->master.torque +
                                                context->slave.torque)));
        veItemOwnerSet(
            node->device->motorTemperature,
            veVariantSn16(&v, higherOf(context->master.motorTemperature,
                                       context->slave.motorTemperature)));
        veItemOwnerSet(
            node->device->controllerTemperature,
            veVariantSn16(&v, higherOf(context->master.controllerTemperature,
                                       context->slave.controllerTemperature)));
    } else {
        veItemOwnerSet(node->device->current,
                       veVariantInvalidType(&v, VE_FLOAT));
        veItemOwnerSet(node->device->power, veVariantInvalidType(&v, VE_SN32));
        veItemOwnerSet(node->device->motorTorque,
                       veVariantInvalidType(&v, VE_FLOAT));

        veItemOwnerSet(node->device->motorTemperature,
                       veVariantSn16(&v, context->master.motorTemperature));
        veItemOwnerSet(
            node->device->controllerTemperature,
            veVariantSn16(&v, context->master.controllerTemperature));
    }
}

static void onReadComplete(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = GET_DMC_CONTEXT(node);
    if (context->isDualDrive) {
        publishDualDrive(node, context);
    } else {
        publishSingleDrive(node, context);
    }
}

// Only a dual drive is read here, and a master at node 127 is never one, so
// the slave's node ID is always valid.
static void readSlave(Node *node, DmcContext *context) {
    un8 slaveNodeId = node->device->nodeId + 1;

    if (!context->slaveConnected) {
        context->slaveProbing = veTrue;
        canOpenReadSdoAsync(slaveNodeId, 0x383e, 0, node, onSlaveProbeResponse,
                            onSlaveProbeError);
        return;
    }

    context->slaveProbing = veFalse;
    canOpenReadSdoAsync(slaveNodeId, 0x383e, 0, node, onSlaveCurrentResponse,
                        onSlaveError);
    canOpenReadSdoAsync(slaveNodeId, 0x411c, 0, node, onSlaveTorqueResponse,
                        onSlaveError);
    canOpenReadSdoAsync(slaveNodeId, 0x3836, 0, node,
                        onSlaveMotorTemperatureResponse, onSlaveError);
    canOpenReadSdoAsync(slaveNodeId, 0x3837, 0, node,
                        onSlaveControllerTemperatureResponse, onSlaveError);
    canOpenReadSdoAsync(slaveNodeId, 0x3840, 0, node, onSlaveFaultCodeResponse,
                        onSlaveError);
    canOpenReadSdoAsync(slaveNodeId, 0x3841, 0, node,
                        onSlaveFaultSubcodeResponse, onSlaveError);
}

static void readRoutine(Node *node) {
    DmcContext *context;

    context = GET_DMC_CONTEXT(node);

    canOpenReadSdoAsync(node->device->nodeId, 0x383f, 0, node,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x383e, 0, node,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x606c, 0, node,
                        onMotorRpmResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3836, 0, node,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x411c, 0, node,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3837, 0, node,
                        onControllerTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3840, 0, node,
                        onFaultCodeResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x3841, 0, node,
                        onFaultSubcodeResponse, onError);

    if (context->isDualDrive) {
        readSlave(node, context);
    }
    canOpenQueueCallbackAsync(node, onReadComplete);
}

static void fastReadRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x606c, 0, node,
                        onMotorRpmResponse, onError);
}

// EMCY payload, see DMC Advanced CAN Open manual V1.10:
// bytes 0-1 emergency error code, byte 2 error register, byte 3 DMC fault
// code, bytes 4-7 DMC fault subcode.
//
// Sigma2N firmware V03.03.01 was not observed to send one for the drive
// inhibit we could provoke, F13. Whether it does for other fault classes is
// unknown, so this is kept alongside the polled path above and shares the
// reported-fault state, so the two cannot announce the same fault twice. A
// slave's EMCY arrives for a node that is not connected and is not handled;
// its faults are covered by polling.
static void onEMCYMessage(Node *node, VeRawCanMsg *message) {
    un16 errorCode;
    un8 faultCode;
    un32 faultSubcode;
    DmcContext *context;

    errorCode = message->mdata[0] | (message->mdata[1] << 8);
    faultCode = message->mdata[3];
    faultSubcode = message->mdata[4] | (message->mdata[5] << 8) |
                   (message->mdata[6] << 16) | ((un32)message->mdata[7] << 24);

    context = GET_DMC_CONTEXT(node);

    if (errorCode == 0) {
        // Fault cleared. Forget the reported fault so that it is announced
        // again if it recurs before the next poll observes the clear. A reset
        // arriving between the 0x3840 and 0x3841 reads of a poll can pair a
        // stale code with a fresh subcode; that window is accepted.
        context->fault.reportedCode = 0;
        context->fault.reportedSubcode = 0;
        return;
    }

    if (faultCode == context->fault.reportedCode &&
        (un16)faultSubcode == context->fault.reportedSubcode) {
        return;
    }

    context->fault.reportedCode = faultCode;
    context->fault.reportedSubcode = (un16)faultSubcode;

    reportFault(node, 0, faultCode, faultSubcode);
}

static void
onDualMotorConfigurationResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    if ((un8)request->response.data != DMC_DUAL_MOTOR_MASTER) {
        return;
    }
    if (node->device->nodeId >= 127) {
        // The slave is at the master's node ID plus one, and there is no node
        // 128. A configuration mistake, so the master is read on its own.
        warning("Dual motor master at node %u has no slave node ID, reading it "
                "as a single drive",
                node->device->nodeId);
        return;
    }

    context = GET_DMC_CONTEXT(node);
    context->isDualDrive = veTrue;
    context->slaveConnected = veTrue;
}

static void onDualMotorConfigurationError(CanOpenPendingSdoRequest *request,
                                          CanOpenError error) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }
    if (error == SDO_READ_ERROR_TIMEOUT) {
        disconnectFromNode(node->device->nodeId);
    }
    // Otherwise the firmware has no 0x2403 and no dual motor support either.
}

static void *createDriverContext(Node *node) {
    DmcContext *context;

    context = _malloc(sizeof(*context));
    CHECK_ALLOC(context);
    memset(context, 0, sizeof(*context));

    canOpenReadSdoAsync(node->device->nodeId, 0x2403, 0, node,
                        onDualMotorConfigurationResponse,
                        onDualMotorConfigurationError);

    return (void *)context;
}

static void destroyDriverContext(Node *node, void *context) { _free(context); }

Driver dmcDriver = {
    .name = "dmc",
    .productId = VE_PROD_ID_DMC_MOTORDRIVE,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = createDriverContext,
    .destroyDriverContext = destroyDriverContext,
    .onEMCYMessage = onEMCYMessage,
};
