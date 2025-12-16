#ifndef __SERVICE_MANAGER_H__
#define __SERVICE_MANAGER_H__

#include <array.h>
#include <node.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>

typedef struct _ServiceManager {
    struct VeDbus *dbus;
    VeItem *root;
    VeItem *scan;
    VeItem *scanProgress;

    VeItem *discoveredNodes;
    Un8Array discoveredNodeIds;
} ServiceManager;

extern ServiceManager serviceManager;

void serviceManagerInit(void);

#endif