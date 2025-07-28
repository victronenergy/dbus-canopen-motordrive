#include <canopen.h>
#include <curtis.h>
#include <device.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <sevcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

static un16 task1sLastUpdate = 0;
static un16 task10sLastUpdate = 0;
static Node nodes[127];

void taskEarlyInit(void) {
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void taskInit(void) {
    memset(nodes, 0, sizeof(nodes));

    localSettingsInit();
    serviceManagerInit();
}

void connectNode(un8 nodeId) {
    un8 name[255];
    un8 length;
    Node *node;
    Driver *driver;

    if (readSegmentedSdo(nodeId, 0x1008, 0, name, &length, 255) != 0) {
        return;
    }
    driver = getDriverForNodeName(name, length);
    if (driver == NULL) {
        return; // Unsupported controller type
    }

    node = &nodes[nodeId - 1];
    node->device = malloc(sizeof(*node->device));
    if (!node->device) {
        error("malloc failed for node device");
        pltExit(5);
    }
    node->device->driver = driver;

    if (createDevice(node->device, nodeId)) {
        free(node->device);
        node->device = NULL;
        return;
    }

    node->connected = veTrue;
}

void disconnectNode(Node *node) {
    destroyDevice(node->device);
    free(node->device);
    node->device = NULL;
    node->connected = veFalse;
}

void task1s() {
    un8 nodeId;
    Node *node;

    if (serviceManager.scan->variant.value.UN8 == 1) {
        scanBus();
    }

    for (nodeId = 1, node = nodes; nodeId <= 127; nodeId += 1, node += 1) {
        if (node->connected) {
            if (node->device->driver->readRoutine(node->device)) {
                disconnectNode(node);
            }
        }
    }
}

void task10s() {
    size_t i;
    un8 *ptr;
    un8 nodeId;
    Node *node;

    for (ptr = serviceManager.discoveredNodeIds.data, i = 0;
         i < serviceManager.discoveredNodeIds.count; i += 1, ptr += 1) {
        nodeId = *ptr;
        node = &nodes[nodeId - 1];
        if (!node->connected) {
            connectNode(nodeId);
        }
    }
}

void taskUpdate(void) {
    VeRawCanMsg msg;

    while (veCanRead(&msg)) {
        // Prevents high CPU usage if nothing else consumes the CAN messages
    }

    if (veTick1ms(&task1sLastUpdate, 1000)) {
        task1s();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

void taskTick(void) {
    un8 nodeId;
    Node *node;

    for (nodeId = 1, node = nodes; nodeId <= 127; nodeId += 1, node += 1) {
        if (node->connected) {
            veItemTick(node->device->root);
        }
    }
    veItemTick(serviceManager.root);
}

char const *pltProgramVersion(void) { return VERSION; }