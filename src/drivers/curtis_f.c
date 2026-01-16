#include <canopen.h>
#include <drivers/curtis_f.h>
#include <localsettings.h>
#include <logger.h>
#include <memory.h>
#include <node.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>
#include <velib/vecan/products.h>

typedef struct {
    un16 faultId;
    un16 faultRecordId;
    char const *error;
} Error;

static Error errorDb[] = {
    {0xFF12, 0x2510, "Controller Overcurrent"},
    {0xFF13, 0x2832, "Current Sensor"},
    {0xFF14, 0x2223, "Precharge Failed"},
    {0xFF15, 0x2141, "Controller Severe Undertemperature"},
    {0xFF16, 0x2142, "Controller Severe Overtemperature"},
    {0xFF17, 0x2120, "Severe B+ Undervoltage"},
    {0xFF17, 0x2122, "Severe KSI Undervoltage"},
    {0xFF18, 0x2130, "Severe B+ Overvoltage"},
    {0xFF18, 0x2132, "Severe KSI Overvoltage"},
    {0xFF19, 0x2133, "Speed Limit Supervision"},
    {0xFF1A, 0x2134, "Motor Not Stopped"},
    {0xFF1B, 0x2109, "Critical OS General"},
    {0xFF1C, 0x210A, "OS General 2"},
    {0xFF1E, 0x210E, "Motor Short"},
    {0xFF1D, 0x2110, "Reset Rejected"},
    {0xFF22, 0x2140, "Controller Overtemperature Cutback"},
    {0xFF23, 0x2121, "Undervoltage Cutback"},
    {0xFF24, 0x2131, "Overvoltage Cutback"},
    {0xFF25, 0x2531, "Ext 5V Supply Failure"},
    {0xFF26, 0x2532, "Ext 12V Supply Failure"},
    {0xFF28, 0x2151, "Motor Temp Hot Cutback"},
    {0xFF29, 0x2150, "Motor Temp Sensor"},
    {0xFF31, 0x2222, "Main Driver"},
    {0xFF32, 0x2320, "EM Brake Driver"},
    {0xFF33, 0x2420, "Pump Driver"},
    {0xFF34, 0x2430, "Load Hold Driver"},
    {0xFF35, 0x2440, "Lower Driver"},
    {0xFF36, 0x2230, "IM Motor Feedback"},
    {0xFF36, 0x2232, "Sin Cos Motor Feedback"},
    {0xFF37, 0x2240, "Motor Open"},
    {0xFF38, 0x2220, "Main Contactor Welded"},
    {0xFF39, 0x2221, "Main Contactor Did Not Close"},
    {0xFF3A, 0x2103, "Motor Setup Needed"},
    {0xFF42, 0x2210, "Throttle Input"},
    {0xFF44, 0x2310, "Brake Input"},
    {0xFF46, 0x2830, "NV Memory Failure"},
    {0xFF47, 0x2211, "HPD Sequencing"},
    {0xFF47, 0x2331, "Emer Rev HPD"},
    {0xFF49, 0x2813, "Parameter Change"},
    {0xFF4A, 0x2817, "EMR Switch Redundancy"},
    {0x6251, 0x2710, "User 1 Fault thru User 32 Fault"},
    {0x6252, 0x2711, "User 2 Fault"},
    {0x6253, 0x2712, "User 3 Fault"},
    {0x6254, 0x2713, "User 4 Fault"},
    {0x6255, 0x2720, "User 5 Fault"},
    {0x6256, 0x2721, "User 6 Fault"},
    {0x6257, 0x2722, "User 7 Fault"},
    {0x6258, 0x2723, "User 8 Fault"},
    {0x6259, 0x2730, "User 9 Fault"},
    {0x6261, 0x2731, "User 10 Fault"},
    {0x6262, 0x2732, "User 11 Fault"},
    {0x6263, 0x2733, "User 12 Fault"},
    {0x6264, 0x2740, "User 13 Fault"},
    {0x6265, 0x2741, "User 14 Fault"},
    {0x6266, 0x2742, "User 15 Fault"},
    {0x6267, 0x2743, "User 16 Fault"},
    {0x625A, 0x2750, "User 17 Fault"},
    {0x625B, 0x2751, "User 18 Fault"},
    {0x625C, 0x2752, "User 19 Fault"},
    {0x625D, 0x2753, "User 20 Fault"},
    {0x625E, 0x2760, "User 21 Fault"},
    {0x625F, 0x2761, "User 22 Fault"},
    {0x626A, 0x2762, "User 23 Fault"},
    {0x626B, 0x2763, "User 24 Fault"},
    {0x626C, 0x2770, "User 25 Fault"},
    {0x626D, 0x2771, "User 26 Fault"},
    {0x626E, 0x2772, "User 27 Fault"},
    {0x626F, 0x2773, "User 28 Fault"},
    {0x627A, 0x2780, "User 29 Fault"},
    {0x627B, 0x2781, "User 30 Fault"},
    {0x627C, 0x2782, "User 31 Fault"},
    {0x627D, 0x2783, "User 32 Fault"},
    {0xFF68, 0x2820, "VCL Run Time Error"},
    {0xFF71, 0x2831, "OS General"},
    {0xFF72, 0x2541, "PDO Timeout"},
    {0xFF73, 0x2231, "Stall Detected"},
    {0xFF77, 0x2840, "Supervision"},
    {0xFF79, 0x2841, "Supervision Input Check"},
    {0xFF82, 0x2542, "PDO Mapping Error"},
    {0xFF83, 0x2835, "Internal Hardware"},
    {0xFF84, 0x211A, "Motor Braking Impaired"},
    {0xFF87, 0x2850, "Motor Characterization"},
    {0xFF88, 0x2234, "Encoder Pulse Error"},
    {0xFF89, 0x2811, "Parameter Out of Range"},
    {0xFF91, 0x2815, "Bad Firmware"},
    {0xFF92, 0x2321, "EM Brake Failed to Set"},
    {0xFF93, 0x2233, "Encoder LOS"},
    {0xFF94, 0x2330, "Emer Rev Timeout"},
    {0xFF96, 0x2450, "Pump BDI"},
    {0xFF99, 0x2812, "Parameter Mismatch"},
    {0xFF9A, 0x2332, "Interlock Braking Supervision"},
    {0xFF9B, 0x2333, "EMR Supervision"},
    {0xFFA1, 0x2160, "Driver 1 Fault"},
    {0xFFA2, 0x2161, "Driver 2 Fault"},
    {0xFFA3, 0x2162, "Driver 3 Fault"},
    {0xFFA4, 0x2163, "Driver 4 Fault"},
    {0xFFA5, 0x2164, "Driver 5 Fault"},
    {0xFFA6, 0x2165, "Driver 6 Fault"},
    {0xFFA7, 0x2166, "Driver 7 Fault"},
    {0xFFA8, 0x2632, "Driver Assignment"},
    {0xFFA9, 0x2169, "Coil Supply"},
    {0xFFB1, 0x2620, "Analog 1 Out Of Range"},
    {0xFFB2, 0x2621, "Analog 2 Out Of Range"},
    {0xFFB3, 0x2622, "Analog 3 Out Of Range"},
    {0xFFB4, 0x2623, "Analog 4 Out Of Range"},
    {0xFFB5, 0x2624, "Analog 5 Out Of Range"},
    {0xFFB6, 0x2625, "Analog 6 Out Of Range"},
    {0xFFB7, 0x2626, "Analog 7 Out Of Range"},
    {0xFFB8, 0x2627, "Analog 8 Out Of Range"},
    {0xFFB9, 0x2628, "Analog 9 Out of Range"},
    {0xFFBB, 0x262A, "Analog 14 Out Of Range"},
    {0xFFBD, 0x262B, "Analog 18 Out of range"},
    {0xFFBE, 0x262C, "Analog 19 Out of range"},
    {0xFFBC, 0x2631, "Analog Assignment"},
    {0xFFC1, 0x2860, "Branding Error"},
    {0xFFC2, 0x2861, "BMS Cutback"},
    {0xFFC5, 0x2629, "PWM Input 10 Out of Range"},
    {0xFFC7, 0x2106, "Analog 31 Out of Range"},
    {0xFFC8, 0x2107, "Invalid CAN Port"},
    {0xFFC9, 0x2108, "VCL Watchdog"},
    {0xFFCB, 0x210C, "PWM Input 28 Out of Range"},
    {0xFFCC, 0x210D, "PWM Input 29 Out of Range"},
    {0xFFCB, 0x2113, "Primary State Error"},
    {0xFFD1, 0x2104, "Lift Input"},
    {0xFFD2, 0x2101, "Phase PWM Mismatch"},
    {0xFFD3, 0x2870, "Hardware Compatibility"},
    {0xFFD4, 0x2105, "Lower Input"},
    {0xFFD6, 0x211C, "Hazardous Movement"},
    {0xFFDD, 0x2114, "IMU Failure"},
};

static Error *findError(un16 faultId, un16 faultRecordId) {
    size_t dbSize = sizeof(errorDb) / sizeof(Error);
    for (size_t i = 0; i < dbSize; i++) {
        if (errorDb[i].faultId == faultId &&
            errorDb[i].faultRecordId == faultRecordId) {
            return &errorDb[i];
        }
    }
    return NULL;
}

static void onSwapMotorDirectionResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    CurtisFContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = (CurtisFContext *)node->device->driverContext;
    context->swapMotorDirection = request->response.data;
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

    current = ((sn32)request->response.data) * 0.1F;

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
    CurtisFContext *context;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    context = (CurtisFContext *)node->device->driverContext;

    rpm = request->response.data;
    if (context->swapMotorDirection == 1) { // Throttle is reversed
        rpm *= -1;
    }

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

static void onError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    disconnectFromNode(node->device->nodeId);
}

static void readRoutine(Node *node) {
    CurtisFContext *context;

    context = (CurtisFContext *)node->device->driverContext;
    if (context->swapMotorDirection == -1) {
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

static void fastReadRoutine(Node *node) {
    canOpenReadSdoAsync(node->device->nodeId, 0x352F, 0, node,
                        onMotorRpmResponse, onError);
}

static void *createDriverContext(Node *node) {
    CurtisFContext *context;

    context = _malloc(sizeof(*context));
    if (!context) {
        error("malloc failed for CurtisFContext");
        pltExit(5);
    }

    context->swapMotorDirection = -1;

    return (void *)context;
}

static void destroyDriverContext(Node *node, void *context) { _free(context); }

static void onEMCYMessage(Node *node, VeRawCanMsg *message) {
    un16 faultId;
    un16 faultRecordId;
    un8 faultType;
    Error *error;
    char notificationTitle[255];
    VeStr deviceName;

    faultId = message->mdata[0] | (message->mdata[1] << 8);
    faultRecordId = message->mdata[3] | (message->mdata[4] << 8);
    faultType = message->mdata[5];

    if (faultId == 0) {
        // No fault
        return;
    }

    error = findError(faultId, faultRecordId);
    if (error != NULL) {
        snprintf(notificationTitle, sizeof(notificationTitle),
                 "%s (fault type: %d)", error->error, faultType);
    } else {
        snprintf(notificationTitle, sizeof(notificationTitle),
                 "Unknown Error 0x%04X Record 0x%04X (fault type: %d)", faultId,
                 faultRecordId, faultType);
    }

    error("EMCY from node %d: %s", node->device->nodeId, notificationTitle);
    getDeviceDisplayName(node->device, &deviceName);
    injectPlatformNotification(NOTIFICATION_TYPE_ERROR, notificationTitle,
                               veStrCStr(&deviceName));
    veStrFree(&deviceName);
}

Driver curtisFDriver = {
    .name = "curtis_f",
    .productId = VE_PROD_ID_CURTIS_MOTORDRIVE,
    .readRoutine = readRoutine,
    .fastReadRoutine = fastReadRoutine,
    .createDriverContext = createDriverContext,
    .destroyDriverContext = destroyDriverContext,
    .onEMCYMessage = onEMCYMessage,
};
