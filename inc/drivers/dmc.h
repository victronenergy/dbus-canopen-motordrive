#ifndef __DMC_H__
#define __DMC_H__

#include <driver.h>
#include <velib/types/ve_item_def.h>

/**
 * Object 0x2403 "Dual motor configuration", parameter M2-5 "DMconfig" in the
 * Sigma2N Traction Advanced Manual V1.11. A 6-phase motor on one shaft is
 * driven by two controllers: a master running the speed loop and a slave
 * following its torque. Only that combination is read as one drive. The other
 * dual motor configurations drive a wheel or shaft each and stay separate
 * devices.
 */
#define DMC_DUAL_MOTOR_MASTER 3
#define DMC_DUAL_MOTOR_SLAVE 4

/**
 * Faults are polled from 0x3840 and 0x3841 rather than taken from EMCY alone.
 * EMCY is edge triggered, so a fault already standing when the driver starts
 * would never be reported, and on Sigma2N firmware V03.03.01 a drive inhibit
 * such as F13 raises no EMCY at all. The fault state remembers what has
 * already been reported so a fault standing for minutes is not announced on
 * every read cycle.
 */
typedef struct _DmcFaultState {
    un8 pendingCode;
    un8 reportedCode;
    un16 reportedSubcode;
} DmcFaultState;

typedef struct _DmcReadings {
    float current;
    float torque;
    sn16 motorTemperature;
    sn16 controllerTemperature;
} DmcReadings;

/**
 * On connection the driver reads 0x2403, and only a dual motor master gets
 * the SlaveNodeId setting, which is filled in by hand. When a slave is
 * configured its values are read in the same cycle, after the master's, and
 * the combined result is published once the slave's last read has answered.
 * The master's readings are held here until then.
 */
typedef struct _DmcContext {
    VeItem *slaveNodeIdSetting;
    un8 slaveNodeId;
    veBool slaveConnected;
    veBool slaveProbing;

    DmcFaultState fault;
    DmcFaultState slaveFault;

    DmcReadings master;
    DmcReadings slave;
} DmcContext;

extern Driver dmcDriver;

#endif
