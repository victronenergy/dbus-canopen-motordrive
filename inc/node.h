#ifndef __NODE_H__
#define __NODE_H__

typedef struct _Node Node;

#include <canopen.h>
#include <device.h>
#include <velib/types/ve_item_def.h>

typedef struct _Node {
    veBool connected;
    Device *device;
} Node;

typedef struct _ConnectionAttempt {
    un8 nodeId;
    un8 name[255];
    un8 length;
    Driver *driver;
} ConnectionAttempt;

void connectToNode(un8 nodeId);
void connectToDiscoveredNodes();
void disconnectFromNode(un8 nodeId);
void readFromConnectedNodes(veBool fast);
void nodesTick();
void nodesInit();
void nodesEmcyHandler(void *context, un8 nodeId, VeRawCanMsg *message);

extern Node nodes[127];

#endif