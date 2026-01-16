#include <canopen.h>
#include <drivers/curtis_e.h>
#include <localsettings.h>
#include <logger.h>
#include <memory.h>
#include <node.h>
#include <platform.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_str.h>
#include <velib/vecan/products.h>

#define RPM_DEADBAND 3
#define CURRENT_DEADBAND 0.1F

typedef struct {
    un8 statusRegister;
    un8 mask;
    char const *error;
} Error;

static Error errorDb[] = {
    /* Status1 */
    {0x01, 0b00000001, "Main Contactor Welded (Code 38)"},
    {0x01, 0b00000010, "Main Contactor Did Not Close (Code 39)"},
    {0x01, 0b00000100, "Pot Low OverCurrent (Code 45)"},
    {0x01, 0b00001000, "Throttle Wiper Low (Code 42)"},
    {0x01, 0b00010000, "Throttle Wiper High (Code 41)"},
    {0x01, 0b00100000, "Pot2 Wiper Low (Code 44)"},
    {0x01, 0b01000000, "Pot2 Wiper High (Code 43)"},
    {0x01, 0b10000000, "EEPROM Failure (Code 46)"},

    /* Status2 */
    {0x02, 0b00000001, "HPD/Sequencing Fault (Code 47)"},
    {0x02, 0b00000010, "Severe B+ Undervoltage (Code 17)"},
    {0x02, 0b00000100, "Severe B+ Overvoltage (Code 18)"},
    {0x02, 0b00001000, "B+ Undervoltage Cutback (Code 23)"},
    {0x02, 0b00010000, "B+ Overvoltage Cutback (Code 24)"},
    {0x02, 0b00100000, "Sin/Cos Sensor Fault (Code 36)"},
    {0x02, 0b01000000, "Controller Overtemp Cutback (Code 22)"},
    {0x02, 0b10000000, "Controller Severe Undertemp (Code 15)"},

    /* Status3 */
    {0x03, 0b00000001, "Controller Severe Overtemp (Code 16)"},
    {0x03, 0b00000010, "Coil1 Driver Open/Short (Code 31)"},
    {0x03, 0b00000100, "Coil2 Driver Open/Short (Code 32)"},
    {0x03, 0b00001000, "Coil3 Driver Open/Short (Code 33)"},
    {0x03, 0b00010000, "Coil4 Driver Open/Short (Code 34)"},
    {0x03, 0b00100000, "PD Open/Short (Code 35)"},
    {0x03, 0b01000000, "Main Open/Short (Code 31)"},
    {0x03, 0b10000000, "EMBrake Open/Short (Code 32)"},

    /* Status4 */
    {0x04, 0b00000001, "Precharge Failed (Code 14)"},
    {0x04, 0b00000010, "Digital Out 6 Overcurrent (Code 26)"},
    {0x04, 0b00000100, "Digital Out 7 Overcurrent (Code 27)"},
    {0x04, 0b00001000, "Controller Overcurrent (Code 12)"},
    {0x04, 0b00010000, "Current Sensor Fault (Code 13)"},
    {0x04, 0b00100000, "Motor Temp Hot Cutback (Code 28)"},
    {0x04, 0b01000000, "Parameter Change Fault (Code 49)"},
    {0x04, 0b10000000, "Motor Open (Code 37)"},

    /* Status5 */
    {0x05, 0b00000001, "External Supply Out of Range (Code 69)"},
    {0x05, 0b00000010, "Motor Temp Sensor Fault (Code 29)"},
    {0x05, 0b00000100, "VCL Run Time Error (Code 68)"},
    {0x05, 0b00001000, "+5V Supply Failure (Code 25)"},
    {0x05, 0b00010000, "OS General (Code 71)"},
    {0x05, 0b00100000, "PDO Timeout (Code 72)"},
    {0x05, 0b01000000, "Encoder Fault (Code 36)"},
    {0x05, 0b10000000, "Stall Detected (Code 73)"},

    /* Status6 */
    {0x06, 0b00000001, "Bad_Calibrations_Fault (Code 82)"},
    {0x06, 0b00000100, "Emer Rev HPD (Code 47)"},
    {0x06, 0b00010000, "Motor Type Fault (Code 89)"},
    {0x06, 0b00100000, "Supervisor Fault (Code 77)"},
    {0x06, 0b01000000, "Motor Characterization Fault (Code 87)"},

    /* Status7 */
    {0x07, 0b00000010, "VCL/OS Mismatch (Code 91)"},
    {0x07, 0b00000100, "EM Brake Failed to Set (Code 92)"},
    {0x07, 0b00001000, "Encoder LOS (Limited Operating Strategy) (Code 93)"},
    {0x07, 0b00010000, "Emer Rev Timeout (Code 94)"},
    {0x07, 0b00100000, "Dual Severe Fault (Code 75)"},
    {0x07, 0b01000000, "Fault On Other Traction Controller (Code 74)"},
    {0x07, 0b10000000, "Illegal Model Number (Code 98)"},

    /* Status8 */
    {0x08, 0b00001000, "Parameter Mismatch (Code 99)"},
    {0x08, 0b00010000, "Severe KSI Undervoltage (Code 17)"},
    {0x08, 0b01000000, "Insulation Resistance Low (Code 76)"},
    {0x08, 0b10000000, "Encoder Pulse Count Fault (Code 88)"},

    /* Status9 */
    {0x09, 0b00000001, "Supervisor Incompatible (Code 78)"},
    {0x09, 0b00001000, "PMAC Commissioning Needed (Code 19)"},
    {0x09, 0b01000000, "Driver Supply (Code 83)"},
};

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

    current = ((sn16)request->response.data) * 0.1F;
    if (current >= -CURRENT_DEADBAND && current <= CURRENT_DEADBAND) {
        current = 0.0F;
    }

    veItemOwnerSet(node->device->current, veVariantFloat(&v, current));
    veItemLocalValue(node->device->voltage, &v);
    veItemOwnerSet(node->device->power,
                   veVariantSn32(&v, (sn32)(v.value.Float * current)));
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

    context = _malloc(sizeof(*context));
    if (!context) {
        error("malloc failed for CurtisEContext");
        pltExit(5);
    }

    context->swapMotorDirection = -1;

    return (void *)context;
}

static void destroyDriverContext(Node *node, void *context) { _free(context); }

static void onEMCYMessage(Node *node, VeRawCanMsg *message) {
    un16 errorCategory;
    un8 statusRegisterStart;
    size_t errorDbSize;
    Error *errorEntry;
    VeStr deviceName;

    errorDbSize = sizeof(errorDb) / sizeof(Error);
    errorCategory = message->mdata[0] | (message->mdata[1] << 8);

    if (errorCategory == 0x1000) {
        statusRegisterStart = 1;
    } else if (errorCategory == 0x1001) {
        statusRegisterStart = 6;
    } else {
        // Unknown error category
        return;
    }

    for (size_t j = 0; j < errorDbSize; j++) {
        errorEntry = &errorDb[j];
        if (errorEntry->statusRegister < statusRegisterStart ||
            errorEntry->statusRegister >= statusRegisterStart + 5) {
            continue;
        }
        if (message
                ->mdata[errorEntry->statusRegister - statusRegisterStart + 3] &
            errorEntry->mask) {
            error("EMCY from node %d: %s", node->device->nodeId,
                  errorEntry->error);
            getDeviceDisplayName(node->device, &deviceName);
            injectPlatformNotification(NOTIFICATION_TYPE_ERROR,
                                       errorEntry->error,
                                       veStrCStr(&deviceName));
            veStrFree(&deviceName);
        }
    }
}

Driver curtisEDriver = {
    .name = "curtis_e",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = createDriverContext,
    .destroyDriverContext = destroyDriverContext,
    .onEMCYMessage = onEMCYMessage,
    .fetchSerialNumber = NULL,
};
