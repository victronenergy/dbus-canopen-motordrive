#ifndef __DRIVER_H__
#define __DRIVER_H__

typedef struct _Driver Driver;

#include <device.h>
#include <velib/base/types.h>

typedef struct _Driver {
    const char *name;
    un16 productId;
    veBool (*getSerialNumber)(un8 nodeId, un32 *serialNumber);
    veBool (*readRoutine)(Device *device);
    void (*onBeforeDbusInit)(Device *device);
    void (*onDestroy)(Device *device);
} Driver;

#endif