#include <canopen.h>
#include <drivers/sevcon.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <velib/vecan/products.h>

typedef struct {
    un16 errorId;
    char const *error;
} Error;

static Error errorDb[] = {
    {0x4441, "Motor Characterisation Mode"},
    {0x4C41, "Too many slaves"},
    {0x5041, "Bad NVM Data"},
    {0x5042, "VPDO Out of Range"},
    {0x5043, "Static Range Error"},
    {0x5044, "Dynamic Range Error"},
    {0x5441, "Incompatible hardware version"},
    {0x5442, "Calibration Fault"},
    {0x4481, "Handbrake Fault (warn)"},
    {0x4881, "Seat Fault"},
    {0x4882, "Two Direction Fault"},
    {0x4883, "SRO Fault"},
    {0x4884, "Sequence Fault"},
    {0x4885, "FS1 Recycle Fault"},
    {0x4886, "Inch Fault"},
    {0x488D, "Belly fault"},
    {0x5081, "Invalid Steer Switches"},
    {0x54C1, "PowerFrame Overvoltage Fault"},
    {0x54C2, "PowerFrame Fault"},
    {0x54C3, "PowerFrame s/c M1 upper"},
    {0x54C4, "PowerFrame s/c M1 lower"},
    {0x54C5, "PowerFrame s/c M2 upper"},
    {0x54C6, "PowerFrame s/c M2 lower"},
    {0x54C7, "PowerFrame s/c M3 upper"},
    {0x54C8, "PowerFrame s/c M3 lower"},
    {0x54C9, "PowerFrame s/c checks incomplete"},
    {0x5101, "Line Contactor o/c"},
    {0x5102, "Line Contactor welded"},
    {0x5103, "Contactor Drive Fault"},
    {0x4546, "No Motor Speed Signal"},
    {0x4548, "Steer Sensor warning"},
    {0x454C, "Electrolyte Low Level"},
    {0x454D, "Electrolyte Cutout Level"},
    {0x4D42, "Motor Open Circuit Fault"},
    {0x4D43, "Motor stalled"},
    {0x458A, "Seat (warning)"},
    {0x458B, "Footbrake (warning)"},
    {0x4981, "Throttle Fault"},
    {0x4982, "E-Brake Wire off"},
    {0x4983, "Direction Change"},
    {0x5181, "Digital Input Wire Off"},
    {0x5182, "Analog Input Wire Off"},
    {0x5183, "Analog Output Over Current"},
    {0x5184, "Analog Output On with No Failsafe"},
    {0x5185, "Analog Output Off with Failsafe"},
    {0x5186, "Analog Output Over Temperature"},
    {0x5187, "Analog Output Under Current"},
    {0x5188, "Analog Output Short Circuit"},
    {0x45C1, "BDI (battery discharge) warning"},
    {0x45C2, "BDI (battery discharge) cutout"},
    {0x45C3, "Low Battery cutout"},
    {0x45C4, "High Battery cutout"},
    {0x45C5, "High Capacitor cutout"},
    {0x45C6, "Vbat below rated min"},
    {0x45C7, "Vbat above rated max"},
    {0x45C8, "Vcap above rated max"},
    {0x45C9, "Vcap cutback for motoring torque"},
    {0x45CA, "Vcap cutback for regen torque"},
    {0x45CE, "Vcap to Vkey (Vbat) difference"},
    {0x4DC3, "Power Supply (keyswitch) Critical"},
    {0x51C2, "Capacitor Precharge Failure"},
    {0x4601, "Device too cold"},
    {0x4602, "Device too hot"},
    {0x4603, "Motor in thermal cutback"},
    {0x4604, "Motor too cold"},
    {0x5201, "Heatsink / device overtemp"},
    {0x4681, "Unit in pre-operational"},
    {0x4682, "IO can't initialise"},
    {0x4683, "RPDO Timeout (warning)"},
    {0x4A81, "RPDO Timeout (drive inhibit)"},
    {0x4E81, "RPDO Timeout (severe)"},
    {0x46C2, "SinCos Tracking Warning"},
    {0x46C3, "Fault Ride Through"},
    {0x46C6, "Encoder PLL Deactivated (warning)"},
    {0x52C1, "Encoder Fault"},
    {0x52C2, "Motor Overcurrent Fault"},
    {0x52C3, "Current Control Fault"},
    {0x52C4, "Motor Overspeed Fault"},
    {0x4B01, "CAN bus-off (drive inhibit)"},
    {0x4F01, "CANopen unexpected slave state"},
    {0x5301, "CAN bus Fault"},
    {0x5302, "CANopen Bootup not received"},
    {0x5304, "CAN Lo-Pri Tx queue overrun"},
    {0x5305, "CAN Hi-Pri Rx queue overrun"},
    {0x5306, "CAN Hi-Pri Tx queue overrun"},
    {0x5308, "CAN bus-off"},
    {0x530A, "CANopen Short PDO received"},
    {0x530B, "CANopen Heartbeat Failed"},
    {0x530C, "CANopen device in wrong state"},
    {0x530F, "CANopen SDO Timeout Error"},
    {0x5319, "Motor slave in wrong state"},
    {0x4741, "Scheduler stack overflow warning "},
    {0x4F41, "Internal SW Fault"},
    {0x4F42, "Out of memory"},
    {0x4F43, "General DSP error"},
    {0x4F44, "Timer Error"},
    {0x4F45, "Queue Error"},
    {0x4F46, "Scheduler Error"},
    {0x4F48, "I/O Internal SW Error"},
    {0x4F49, "GIO Internal SW Error"},
    {0x4F4C, "OBD Internal SW Error"},
    {0x4F4D, "VehApp Internal SW Error"},
    {0x4F4E, "DMC Internal SW Error"},
    {0x4F4F, "TracApp Internal SW Error"},
    {0x4F53, "App Manager Internal SW Error"},
    {0x4F54, "Autozero Range Error"},
    {0x4F55, "DSP motor parameter error"},
    {0x5343, "Fault List Overflow"},
    {0x5345, "Scheduler stack Overflow"},
    {0x4781, "CANopen anon EMCY level 1"},
    {0x4B81, "CANopen anon EMCY level 2"},
    {0x4F81, "CANopen anon EMCY level 3"},
    {0x5381, "CANopen anon EMCY level 4"},
    {0x5781, "CANopen anon EMCY level 5"},
    {0x47C1, "Vehicle Service Required"},
};

static Error *findError(un16 errorId) {
    size_t dbSize = sizeof(errorDb) / sizeof(Error);
    for (size_t i = 0; i < dbSize; i++) {
        if (errorDb[i].errorId == errorId) {
            return &errorDb[i];
        }
    }
    return NULL;
}

static void onBatteryVoltageResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;
    Node *node;
    float voltage;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    voltage = request->response.data * 0.0625F;

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

    current = ((sn16)request->response.data) * 0.0625F;

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

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    rpm = request->response.data;

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

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->motorTemperature,
                   veVariantSn16(&v, (sn16)request->response.data));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->motorTorque,
                   veVariantUn16(&v, request->response.data));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantSn16(&v, (sn16)request->response.data));
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
    canOpenReadSdoAsync(node->device->nodeId, 0x5100, 1, node,
                        onBatteryVoltageResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x5100, 2, node,
                        onBatteryCurrentResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x606c, 0, node,
                        onMotorRpmResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x4600, 3, node,
                        onMotorTemperatureResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x4602, 0xC, node,
                        onMotorTorqueResponse, onError);
    canOpenReadSdoAsync(node->device->nodeId, 0x5100, 4, node,
                        onControllerTemperatureResponse, onError);
}

static void fastReadRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x606c, 0, node,
                        onMotorRpmResponse, onError);
}

static void onEMCYMessage(Node *node, VeRawCanMsg *message) {
    un16 errorId;
    un32 data;
    Error *error;
    char notificationTitle[255];
    VeStr deviceName;

    errorId = message->mdata[3] | (message->mdata[4] << 8);
    data = message->mdata[5] | (message->mdata[6] << 8) |
           (message->mdata[7] << 16);

    error = findError(errorId);
    if (error == NULL || data == 0) {
        return;
    }
    snprintf(notificationTitle, sizeof(notificationTitle), "%s", error->error);
    error("EMCY from node %d: %s", node->device->nodeId, notificationTitle);
    getDeviceDisplayName(node->device, &deviceName);
    injectPlatformNotification(NOTIFICATION_TYPE_ERROR, notificationTitle,
                               veStrCStr(&deviceName));
    veStrFree(&deviceName);
}

Driver sevconDriver = {
    .name = "sevcon",
    .productId = VE_PROD_ID_SEVCON_MOTORDRIVE,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = NULL,
    .destroyDriverContext = NULL,
    .onEMCYMessage = onEMCYMessage,
    .fetchSerialNumber = NULL,
};