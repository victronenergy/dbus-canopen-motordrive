#ifndef __DMC_H__
#define __DMC_H__

#include <driver.h>

/**
 * Faults are polled from 0x3840 and 0x3841 rather than taken from EMCY alone.
 * EMCY is edge triggered, so a fault already standing when the driver starts
 * would never be reported, and on Sigma2N firmware V03.03.01 a drive inhibit
 * such as F13 raises no EMCY at all. This context remembers what has already
 * been reported so a fault standing for minutes is not announced on every
 * read cycle.
 */
typedef struct _DmcContext {
    un8 pendingFaultCode;
    un8 reportedFaultCode;
    un16 reportedFaultSubcode;
} DmcContext;

extern Driver dmcDriver;

#endif
