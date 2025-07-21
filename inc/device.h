#ifndef __DEVICE_H__
#define __DEVICE_H__

typedef struct _Device Device;

#include <driver.h>
#include <velib/types/variant_print.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/utils/ve_item_utils.h>

typedef struct _Device {
    un8 nodeId;

    struct VeDbus *dbus;

    char identifier[64];
    sn32 deviceInstance;
    un32 serialNumber;

    VeItem *root;
    VeItem *voltage;
    VeItem *current;
    VeItem *motorRpm;
    VeItem *motorDirection;
    VeItem *motorTemperature;
    VeItem *controllerTemperature;
    VeItem *motorDirectionInverted;

    Driver *driver;
    void *driverContext;
} Device;

veBool createDevice(Device *device, un8 nodeId);
void destroyDevice(Device *device);

#endif