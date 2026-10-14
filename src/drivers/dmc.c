#include <canopen.h>
#include <drivers/dmc.h>
#include <localsettings.h>
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

static struct VeSettingProperties slaveNodeIdType = {
    .type = VE_SN32,
    .def.value.SN32 = 0,
    .min.value.SN32 = 0,
    .max.value.SN32 = 127,
};

// 0x2403 per node, as read by discovery.
static un8 dualMotorConfigurations[127];
static veBool dualMotorConfigurationKnown[127];

void dmcRecordDualMotorConfiguration(un8 nodeId, un8 configuration) {
    dualMotorConfigurations[nodeId - 1] = configuration;
    dualMotorConfigurationKnown[nodeId - 1] = veTrue;
}

void dmcResetDualMotorConfigurations(void) {
    memset(dualMotorConfigurations, 0, sizeof(dualMotorConfigurations));
    memset(dualMotorConfigurationKnown, 0, sizeof(dualMotorConfigurationKnown));
}

static veBool hasDualMotorConfiguration(un8 nodeId, un8 configuration) {
    return dualMotorConfigurationKnown[nodeId - 1] &&
           dualMotorConfigurations[nodeId - 1] == configuration;
}

// The only dual motor slave discovery has seen. With two or more there is no
// telling which belongs to which master, so none is chosen.
static un8 findOnlySlave(void) {
    un8 nodeId;
    un8 found;

    found = 0;
    for (nodeId = 1; nodeId <= 127; nodeId += 1) {
        if (hasDualMotorConfiguration(nodeId, DMC_DUAL_MOTOR_SLAVE)) {
            if (found != 0) {
                return 0;
            }
            found = nodeId;
        }
    }
    return found;
}

// SlaveNodeId is 0 until a slave is known. It is filled in once, when this
// controller is a dual motor master and discovery found exactly one slave, and
// then kept: after a restart the slave is not scanned again, because it was
// never added to DiscoveredNodes. Any other value is taken as configured.
static un8 resolveSlaveNodeId(Node *node, DmcContext *context) {
    VeVariant v;
    un8 nodeId;
    un8 slaveNodeId;

    nodeId = node->device->nodeId;
    veItemLocalValue(context->slaveNodeIdSetting, &v);
    if (!veVariantIsValid(&v)) {
        // Not loaded from localsettings yet.
        return 0;
    }
    if (v.value.SN32 != 0) {
        if (v.value.SN32 < 1 || v.value.SN32 > 127 || v.value.SN32 == nodeId) {
            return 0;
        }
        return (un8)v.value.SN32;
    }
    if (!hasDualMotorConfiguration(nodeId, DMC_DUAL_MOTOR_MASTER)) {
        return 0;
    }

    slaveNodeId = findOnlySlave();
    // The local value only changes once localsettings has answered, so write
    // it on the cycle the slave is first found rather than on every cycle.
    if (slaveNodeId != 0 && slaveNodeId != context->slaveNodeId) {
        info("Node %u reads dual motor slave node %u", nodeId, slaveNodeId);
        veItemSet(context->slaveNodeIdSetting, veVariantSn32(&v, slaveNodeId));
    }
    return slaveNodeId;
}

static Error *findError(un8 faultCode) {
    size_t dbSize = sizeof(errorDb) / sizeof(Error);
    for (size_t i = 0; i < dbSize; i++) {
        if (errorDb[i].faultCode == faultCode) {
            return &errorDb[i];
        }
    }
    return NULL;
}

static DmcContext *getContext(Node *node) {
    return (DmcContext *)node->device->driverContext;
}

static sn16 higherOf(sn16 a, sn16 b) { return a > b ? a : b; }

// Half of the drive's current, power or torque would read as the whole drive,
// so while the slave is missing they are published invalid instead.
static void invalidateShares(Node *node) {
    VeVariant v;

    veItemOwnerSet(node->device->current, veVariantInvalidType(&v, VE_FLOAT));
    veItemOwnerSet(node->device->power, veVariantInvalidType(&v, VE_SN32));
    veItemOwnerSet(node->device->motorTorque,
                   veVariantInvalidType(&v, VE_FLOAT));
}

static void publishCombined(Node *node, DmcContext *context) {
    VeVariant v;
    Device *device;
    float current;
    float voltage;

    device = node->device;

    // Each controller reports its own share of the battery current and of
    // the torque, so the drive's values are their sums. Speed and voltage are
    // the master's, and the temperature that matters is the hotter one.
    current = context->master.current + context->slave.current;
    veItemOwnerSet(device->current, veVariantFloat(&v, current));

    veItemLocalValue(device->voltage, &v);
    voltage = v.value.Float;
    veItemOwnerSet(device->power, veVariantSn32(&v, (sn32)(voltage * current)));

    veItemOwnerSet(device->motorTorque,
                   veVariantFloat(&v, fabsf(context->master.torque +
                                            context->slave.torque)));

    veItemOwnerSet(
        device->motorTemperature,
        veVariantSn16(&v, higherOf(context->master.motorTemperature,
                                   context->slave.motorTemperature)));
    veItemOwnerSet(
        device->controllerTemperature,
        veVariantSn16(&v, higherOf(context->master.controllerTemperature,
                                   context->slave.controllerTemperature)));
}

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
    if (getContext(node)->slaveNodeId != 0) {
        // Power follows the combined current, or stays invalid while the
        // slave is missing.
        return;
    }
    veItemLocalValue(node->device->current, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)(voltage * v.value.Float)));
}

static void onBatteryCurrentResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    DmcContext *context;
    float current;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = getContext(node);
    current = ((sn16)request->response.data) * 0.1F;
    context->master.current = current;

    if (context->slaveNodeId != 0) {
        if (context->slaveMissing) {
            invalidateShares(node);
        }
        return;
    }

    veItemOwnerSet(node->device->current, veVariantFloat(&v, current));

    veItemLocalValue(node->device->voltage, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)(v.value.Float * current)));
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
    VeVariant v;
    sn16 temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = getContext(node);
    temperature = ((un8)request->response.data) - TEMPERATURE_OFFSET;
    context->master.motorTemperature = temperature;
    if (context->slaveNodeId != 0 && !context->slaveMissing) {
        return;
    }

    veItemOwnerSet(node->device->motorTemperature,
                   veVariantSn16(&v, temperature));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;
    VeVariant v;
    float torque;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = getContext(node);
    torque = ((sn16)request->response.data) * 0.1F;
    context->master.torque = torque;
    if (context->slaveNodeId != 0) {
        return;
    }

    veItemOwnerSet(node->device->motorTorque,
                   veVariantFloat(&v, fabsf(torque)));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;
    VeVariant v;
    sn16 temperature;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = getContext(node);
    temperature = ((un8)request->response.data) - TEMPERATURE_OFFSET;
    context->master.controllerTemperature = temperature;
    if (context->slaveNodeId != 0 && !context->slaveMissing) {
        return;
    }

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantSn16(&v, temperature));
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

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    getContext(node)->fault.pendingCode = (un8)request->response.data;
}

static void onFaultSubcodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    handleFaultSubcode(node, &getContext(node)->fault, 0,
                       (un16)request->response.data);
}

// Slave reads carry the master's node as their context. Once one has failed
// in a cycle, the rest of that cycle's slave reads are ignored.
static DmcContext *getSlaveReadContext(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return NULL;
    }
    context = getContext(node);
    if (context->slaveMissing) {
        return NULL;
    }
    return context;
}

static void onSlaveCurrentResponse(CanOpenPendingSdoRequest *request) {
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    context->slave.current = ((sn16)request->response.data) * 0.1F;
}

static void onSlaveTorqueResponse(CanOpenPendingSdoRequest *request) {
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    context->slave.torque = ((sn16)request->response.data) * 0.1F;
}

static void onSlaveMotorTemperatureResponse(CanOpenPendingSdoRequest *request) {
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    context->slave.motorTemperature =
        ((un8)request->response.data) - TEMPERATURE_OFFSET;
}

static void
onSlaveControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    context->slave.controllerTemperature =
        ((un8)request->response.data) - TEMPERATURE_OFFSET;
}

static void onSlaveFaultCodeResponse(CanOpenPendingSdoRequest *request) {
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    context->slaveFault.pendingCode = (un8)request->response.data;
}

// The slave's last read of the cycle, so everything is in.
static void onSlaveFaultSubcodeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    DmcContext *context;

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    node = (Node *)request->context;

    publishCombined(node, context);
    handleFaultSubcode(node, &context->slaveFault, request->nodeId,
                       (un16)request->response.data);
}

static void onSlaveError(CanOpenPendingSdoRequest *request,
                         CanOpenError error) {
    Node *node;
    DmcContext *context;
    VeVariant v;
    VeStr deviceName;
    char description[255];

    context = getSlaveReadContext(request);
    if (context == NULL) {
        return;
    }
    node = (Node *)request->context;

    context->slaveMissing = veTrue;
    warning("Slave node %u of node %u is not responding", request->nodeId,
            node->device->nodeId);

    // The master's readings of this cycle are already in, so its own
    // temperatures can still be shown. They are a lower bound for the drive.
    invalidateShares(node);
    veItemOwnerSet(node->device->motorTemperature,
                   veVariantSn16(&v, context->master.motorTemperature));
    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantSn16(&v, context->master.controllerTemperature));

    getDeviceDisplayName(node->device, &deviceName);
    snprintf(description, sizeof(description), "%s, slave node %u",
             veStrCStr(&deviceName), request->nodeId);
    queueNotification(node->device->nodeId, NOTIFICATION_TYPE_WARNING,
                      "Slave controller not responding", description);
    veStrFree(&deviceName);
}

static void onSlaveProbeResponse(CanOpenPendingSdoRequest *request) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    info("Slave node %u of node %u is responding again", request->nodeId,
         node->device->nodeId);
    getContext(node)->slaveMissing = veFalse;
}

static void onSlaveProbeError(CanOpenPendingSdoRequest *request,
                              CanOpenError error) {}

static void readSlave(Node *node, DmcContext *context) {
    un8 slaveNodeId;

    slaveNodeId = context->slaveNodeId;
    if (context->slaveMissing) {
        // A single read per cycle until the slave answers again, rather than
        // a time out on every one of its reads.
        canOpenReadSdoAsync(slaveNodeId, 0x383e, 0, node, onSlaveProbeResponse,
                            onSlaveProbeError);
        return;
    }

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
    un8 slaveNodeId;

    context = getContext(node);
    slaveNodeId = resolveSlaveNodeId(node, context);
    if (slaveNodeId != context->slaveNodeId) {
        context->slaveNodeId = slaveNodeId;
        context->slaveMissing = veFalse;
        memset(&context->slaveFault, 0, sizeof(context->slaveFault));
    }

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

    if (slaveNodeId != 0) {
        readSlave(node, context);
    }
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

    context = getContext(node);

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

static void *createDriverContext(Node *node) {
    DmcContext *context;
    char settingsPath[128];

    context = _malloc(sizeof(*context));
    CHECK_ALLOC(context);
    memset(context, 0, sizeof(*context));

    snprintf(settingsPath, sizeof(settingsPath), "Settings/Devices/%s",
             node->device->identifier);
    context->slaveNodeIdSetting = veItemCreateSettingsProxy(
        localSettings, settingsPath, node->device->root, "SlaveNodeId",
        veVariantFmt, &veUnitNone, &slaveNodeIdType);

    return (void *)context;
}

// The setting item lives in the device tree and goes with it.
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
