#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>

typedef struct {
    struct VeDbus *dbus;

    sn32 deviceInstance;
    un32 serialNumber;

    VeItem *root;
    VeItem voltage;
    VeItem current;
    VeItem rpm;
    VeItem direction;
    VeItem temperature;
} Device;

void createDevice(Device *device, un32 serialNumber);
void destroyDevice(Device *device);

#endif