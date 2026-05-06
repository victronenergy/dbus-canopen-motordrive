#include <canopen.h>
#include <drivers/sevcon.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <notification.h>
#include <stdio.h>
#include <stdlib.h>
#include <velib/vecan/products.h>

typedef struct {
    un16 errorId;
    char const *error;
} Error;

static Error errorDb[] = {{0x4441, "Motor Characterisation Mode"},
                          {0x4481, "Handbrake Fault (warn)"},
                          {0x4541, "Fan Fault"},
                          {0x4542, "Device test waiting"},
                          {0x4542, "Low Oil"},
                          {0x4543, "Hydraulic Filter"},
                          {0x4544, "Pump Current Low"},
                          {0x4545, "Isolation Fault"},
                          {0x4546, "No Motor Speed Signal"},
                          {0x4547, "Tow Mode Active"},
                          {0x4548, "Steer Sensor warning"},
                          {0x4549, "Pulsed Enable signal not received"},
                          {0x454a, "Bridge Enable delayed warn"},
                          {0x454b, "MOSFET s/c tests waiting"},
                          {0x454c, "Electrolyte Low Level"},
                          {0x454d, "Electrolyte Cutout Level"},
                          {0x454e, "Power Limit cutback (warning)"},
                          {0x4581, "Throttle Fault (warning)"},
                          {0x4582, "Safety Case 1"},
                          {0x4583, "Safety Case 2"},
                          {0x4584, "Analogue Output Over Current (warn)"},
                          {0x4585, "Analogue Output Off with Failsafe (warn)"},
                          {0x4586, "Analogue Output Over Temperature (warn)"},
                          {0x4587, "Analogue Output Under Current (warn)"},
                          {0x4588, "Analogue Output Short Circuit (warn)"},
                          {0x4589, "Analogue supply (warn)"},
                          {0x458a, "Seat (warning)"},
                          {0x458b, "Footbrake (warning)"},
                          {0x45c1, "BDI (battery discharge) warning"},
                          {0x45c2, "BDI (battery discharge) cutout"},
                          {0x45c3, "Low Battery cutout"},
                          {0x45c4, "High Battery cutout"},
                          {0x45c5, "High Capacitor cutout"},
                          {0x45c6, "Vbat below rated min"},
                          {0x45c7, "Vbat above rated max"},
                          {0x45c8, "Vcap above rated max"},
                          {0x45c9, "Vcap cutback for motoring torque"},
                          {0x45ca, "Vcap cutback for regen torque"},
                          {0x45cb, "Mains Under Voltage"},
                          {0x45cc, "Mains Over Voltage"},
                          {0x45cd, "KL15 / KL30 Too Low"},
                          {0x45ce, "Vcap to Vkey (Vbat) difference"},
                          {0x4601, "Device too cold"},
                          {0x4602, "Device too hot"},
                          {0x4603, "Motor in thermal cutback"},
                          {0x4604, "Motor too cold"},
                          {0x4605, "Motor Thermistor Wiring"},
                          {0x4681, "Unit in pre-operational"},
                          {0x4682, "IO can't initialise"},
                          {0x4683, "RPDO Timeout (warning)"},
                          {0x46c1, "Encoder Alignment Warning"},
                          {0x46c2, "SinCos Tracking Warning"},
                          {0x46c3, "Fault Ride Through"},
                          {0x46c4, "Induction Motor Pull-Out Warning"},
                          {0x46c5, "Stator Resistance Error"},
                          {0x46c6, "Encoder PLL Deactivated (warning)"},
                          {0x4701, "CAN warning"},
                          {0x4702, "Customer specific CAN protocol warning"},
                          {0x4741, "Scheduler stack overflow warning "},
                          {0x4742, "Internal supply out of range warning"},
                          {0x4743, "CAN Protocol error warning"},
                          {0x4781, "CANopen anon EMCY level 1"},
                          {0x4782, "24V Supply Low"},
                          {0x4783, "24V Supply High"},
                          {0x47c1, "Vehicle Service Required"},
                          {0x47c2, "CAN bus off warning"},
                          {0x47c3, "Protocol CAN off warning"},
                          {0x47c4, "CAN HPRX warning"},
                          {0x47c5, "CAN HPTX warning"},
                          {0x47c6, "Interlock loop broken"},
                          {0x47c7, "Pump oil level low"},
                          {0x47c8, "Pump oil temperature"},
                          {0x47c9, "Active Short Circuit"},
                          {0x4881, "Seat Fault"},
                          {0x4882, "Two Direction Fault"},
                          {0x4883, "SRO Fault"},
                          {0x4884, "Sequence Fault"},
                          {0x4885, "FS1 Recycle Fault"},
                          {0x4886, "Inch Fault"},
                          {0x4887, "Overload Fault"},
                          {0x4888, "Raised and Tilted Fault"},
                          {0x4889, "Pothole Fault"},
                          {0x488a, "Traction Inhibit Fault"},
                          {0x488b, "Illegal Mode Change Fault"},
                          {0x488c, "Tilt Sensor Fault"},
                          {0x488d, "Belly fault"},
                          {0x488e, "Momentary direction fault"},
                          {0x488f, "Sensorless Startup"},
                          {0x4941, "Motor Overspeed"},
                          {0x4942, "PST Fault"},
                          {0x4943, "PFC Fault"},
                          {0x4944, "Boost Fault"},
                          {0x4981, "Throttle Fault"},
                          {0x4982, "E-Brake Wire off"},
                          {0x4983, "Direction Change"},
                          {0x49c1, "ORFET fault"},
                          {0x49c2, "Entering Cutback"},
                          {0x4a01, "Cutback"},
                          {0x4a81, "RPDO Timeout (drive inhibit)"},
                          {0x4b01, "CAN bus-off (drive inhibit)"},
                          {0x4b02, "Ren Data"},
                          {0x4b03, "IO Data Error"},
                          {0x4b04, "Customer Protocol Error"},
                          {0x4b05, "CAN protocol lost message 1"},
                          {0x4b06, "CAN protocol lost message 2"},
                          {0x4b07, "CAN protocol invalid signal 1"},
                          {0x4b08, "CAN protocol invalid signal 2"},
                          {0x4b81, "CANopen anon EMCY level 2"},
                          {0x4b89, "Motor Overtemp"},
                          {0x4b8a, "Device Undertemperature"},
                          {0x4c41, "Too many slaves"},
                          {0x4c81, "HVIL loop broken"},
                          {0x4d01, "Circuit Breaker Open"},
                          {0x4d02, "Circuit Breaker Welded"},
                          {0x4d03, "DC Link Collapsed"},
                          {0x4d04, "Circuit Breaker Timeout"},
                          {0x4d41, "Motor Isolation Fault"},
                          {0x4d42, "Motor Open Circuit Fault"},
                          {0x4d43, "Motor stalled"},
                          {0x4d81, "Pin strapping failed"},
                          {0x4dc3, "Power Supply (keyswitch) Critical"},
                          {0x4e81, "RPDO Timeout (severe)"},
                          {0x4ec1, "Can't Establish Field Current"},
                          {0x4ec2, "Pulsing Disabled"},
                          {0x4f01, "CANopen unexpected slave state"},
                          {0x4f02, "EMCY send failed"},
                          {0x4f41, "Internal SW Fault"},
                          {0x4f42, "Out of memory"},
                          {0x4f43, "General DSP error"},
                          {0x4f44, "Timer Error"},
                          {0x4f45, "Queue Error"},
                          {0x4f46, "Scheduler Error"},
                          {0x4f47, "DSP Heartbeat Error"},
                          {0x4f48, "I/O Internal SW Error"},
                          {0x4f49, "GIO Internal SW Error"},
                          {0x4f4a, "LCM SS Error"},
                          {0x4f4b, "LCP SS Error"},
                          {0x4f4c, "OBD Internal SW Error"},
                          {0x4f4d, "VehApp Internal SW Error"},
                          {0x4f4e, "DMC Internal SW Error"},
                          {0x4f4f, "TracApp Internal SW Error"},
                          {0x4f50, "New Powerframe Detected"},
                          {0x4f51, "DSP Not Detected"},
                          {0x4f52, "DSP / IOP Comms Error"},
                          {0x4f53, "App Manager Internal SW Error"},
                          {0x4f54, "Autozero Range Error"},
                          {0x4f55, "DSP motor parameter error"},
                          {0x4f56, "Motor in wrong direction"},
                          {0x4f57, "Motor stalled"},
                          {0x4f58, "DCDC SS Error"},
                          {0x4f81, "CANopen anon EMCY level 3"},
                          {0x5041, "Bad NVM Data"},
                          {0x5042, "VPDO Out of Range"},
                          {0x5043, "Static Range Error"},
                          {0x5044, "Dynamic Range Error"},
                          {0x5045, "Auto-configuration Fault"},
                          {0x5046, "Voltage autoconfig error"},
                          {0x5081, "Invalid Steer Switches"},
                          {0x5101, "Line Contactor o/c"},
                          {0x5102, "Line Contactor welded"},
                          {0x5103, "Contactor Drive Fault"},
                          {0x5141, "Beltloader Fault"},
                          {0x5142, "Ren Signal"},
                          {0x5143, "VERLOG"},
                          {0x514e, "Power Limit Cutback"},
                          {0x5181, "Digital Input Wire Off"},
                          {0x5182, "Analog Input Wire Off"},
                          {0x5183, "Analog Output Over Current"},
                          {0x5184, "Analog Output On with No Failsafe"},
                          {0x5185, "Analog Output Off with Failsafe"},
                          {0x5186, "Analog Output Over Temperature"},
                          {0x5187, "Analog Output Under Current"},
                          {0x5188, "Analog Output Short Circuit"},
                          {0x51c1, "Power Supply Interrupt"},
                          {0x51c2, "Capacitor Precharge Failure"},
                          {0x51c3, "KL15/30 Too High"},
                          {0x5201, "Heatsink / device overtemp"},
                          {0x52c1, "Encoder Fault"},
                          {0x52c2, "Motor Overcurrent Fault"},
                          {0x52c3, "Current Control Fault"},
                          {0x52c4, "Motor Overspeed Fault"},
                          {0x52c5, "Encoder Alignment Severe"},
                          {0x52c6, "di / dt fault (suspected s/c)"},
                          {0x52c7, "Vcap Overvotlage (software measurement)"},
                          {0x52c8, "Device overcurrent"},
                          {0x5301, "CAN bus Fault"},
                          {0x5302, "CANopen Bootup not received"},
                          {0x5303, "LPRX queue overrun"},
                          {0x5304, "CAN Lo-Pri Tx queue overrun"},
                          {0x5305, "CAN Hi-Pri Rx queue overrun"},
                          {0x5306, "CAN Hi-Pri Tx queue overrun"},
                          {0x5307, "CAN overrun"},
                          {0x5308, "CAN bus-off"},
                          {0x5309, "Nodeguarding Failed"},
                          {0x530a, "CANopen Short PDO received"},
                          {0x530b, "CANopen Heartbeat Failed"},
                          {0x530c, "CANopen device in wrong state"},
                          {0x530d, "CAN ESTAT set"},
                          {0x530e, "SDO Handle Error"},
                          {0x530f, "CANopen SDO Timeout Error"},
                          {0x5310, "SDO Abort Error"},
                          {0x5311, "SDO State Error"},
                          {0x5312, "SDO Toggle Error"},
                          {0x5313, "SDO Rx Error"},
                          {0x5314, "SDO Length Error"},
                          {0x5315, "SDO Tx Error"},
                          {0x5316, "SDO unknown event"},
                          {0x5317, "SDO Bad Source"},
                          {0x5318, "SDO bad error number"},
                          {0x5319, "Motor slave in wrong state"},
                          {0x531a, "Ren Protocol"},
                          {0x5341, "Invalid DSP Protocol"},
                          {0x5342, "OSC Watchdog Fault"},
                          {0x5343, "Fault List Overflow"},
                          {0x5344, "SPI Comms Fault"},
                          {0x5345, "Scheduler stack Overflow"},
                          {0x5346, "Internal supply out of range fault"},
                          {0x5381, "CANopen anon EMCY level 4"},
                          {0x5383, "Boost over voltage"},
                          {0x5384, "Input under voltage"},
                          {0x5385, "Input over voltage"},
                          {0x5386, "Output over voltage"},
                          {0x5387, "Output under Voltage"},
                          {0x5388, "Boost under Voltage"},
                          {0x5389, "Motor Over temperature"},
                          {0x538a, "Device under temperatrue"},
                          {0x5441, "Incompatible hardware version"},
                          {0x5442, "Calibration Fault"},
                          {0x54c1, "PowerFrame Overvoltage Fault"},
                          {0x54c2, "PowerFrame Fault"},
                          {0x54c3, "PowerFrame s/c M1 upper"},
                          {0x54c4, "PowerFrame s/c M1 lower"},
                          {0x54c5, "PowerFrame s/c M2 upper"},
                          {0x54c6, "PowerFrame s/c M2 lower"},
                          {0x54c7, "PowerFrame s/c M3 upper"},
                          {0x54c8, "PowerFrame s/c M3 lower"},
                          {0x54c9, "PowerFrame s/c checks incomplete"},
                          {0x54ca, "Pump MOSFET s/c"},
                          {0x54ca, "IGBT M1 Low Driver Fail"},
                          {0x54cb, "IGBT M1 High Driver Fail"},
                          {0x54cc, "IGBT M2 Low Driver Fail"},
                          {0x54cd, "IGBT M2 High Driver Fail"},
                          {0x54ce, "IGBT M3 Low Driver Fail"},
                          {0x54cf, "IGBT M3 High Driver Fail"},
                          {0x5741, "Invalid Powerframe Rating"},
                          {0x5781, "CANopen anon EMCY level 5"},
                          {0x5782, "Boost Enable Mismatch"}};

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

    voltage = ((un16)request->response.data) * 0.0625F;

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
                   veVariantSn16(&v, request->response.data));
}

static void onMotorTorqueResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->motorTorque,
                   veVariantSn16(&v, request->response.data));
}

static void onControllerTemperatureResponse(CanOpenPendingSdoRequest *request) {
    Node *node;
    VeVariant v;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemOwnerSet(node->device->controllerTemperature,
                   veVariantSn8(&v, request->response.data));
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
    queueNotification(node->device->nodeId, NOTIFICATION_TYPE_ERROR,
                      notificationTitle, veStrCStr(&deviceName));
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
};