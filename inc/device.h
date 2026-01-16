#ifndef __DEVICE_H__
#define __DEVICE_H__

typedef struct _Device Device;

#include <driver.h>
#include <velib/types/variant_print.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/types/ve_str.h>
#include <velib/utils/ve_item_utils.h>

typedef struct _Device {
    un8 nodeId;

    struct VeDbus *dbus;

    char identifier[64];
    sn32 deviceInstance;
    char serialNumber[16];

    VeItem *root;
    VeItem *voltage;
    VeItem *current;
    VeItem *power;
    VeItem *motorRpm;
    VeItem *motorDirection;
    VeItem *motorTemperature;
    VeItem *motorTorque;
    VeItem *controllerTemperature;
    VeItem *coolantTemperature;
    VeItem *motorDirectionInverted;
    VeItem *customName;

    Driver *driver;
    void *driverContext;
} Device;

void getDeviceDisplayName(Device *device, VeStr *out);
void createDevice(Device *device, un8 nodeId, const char *serialNumber);
void destroyDevice(Device *device);

#endif