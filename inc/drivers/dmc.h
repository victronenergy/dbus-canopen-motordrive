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
 * devices. The slave is always at the master's node ID plus one.
 */
#define DMC_DUAL_MOTOR_MASTER 3
#define DMC_DUAL_MOTOR_SLAVE 4

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
 * 0x2403 is read on connection. A master is read together with its slave in
 * the same cycle, and the cycle's values are published once all its reads
 * have answered.
 */
typedef struct _DmcContext {
    // Set when 0x2403 reads DMC_DUAL_MOTOR_MASTER.
    veBool isDualDrive;
    // Cleared when a slave read fails, set again when a probe answers.
    veBool slaveConnected;
    // Set for a cycle that only probes the slave, so it read no values.
    veBool slaveProbing;

    DmcFaultState fault;
    DmcFaultState slaveFault;

    DmcReadings master;
    DmcReadings slave;
} DmcContext;

extern Driver dmcDriver;

#endif
