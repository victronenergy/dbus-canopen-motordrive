#ifndef __NODE_H__
#define __NODE_H__

#include <device.h>
#include <velib/types/ve_item_def.h>

typedef struct _Node {
    veBool connected;
    Device *device;
} Node;

Driver *getDriverForNodeName(un8 *name, un8 length);

#endif