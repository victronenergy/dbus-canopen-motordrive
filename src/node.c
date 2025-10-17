#include <drivers/curtis.h>
#include <drivers/sevcon.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>

static Node nodes[127];

Driver *getDriverForNodeName(un8 *name, un8 length) {
    if (length >= 4 && name[0] == 'G' && name[1] == 'e' && name[2] == 'n' &&
        name[3] == '4') {
        return &sevconDriver;
    } else if (length >= 4 && name[0] == 'A' && name[1] == 'C' &&
               name[2] == ' ' && name[3] == 'F') {
        return &curtisDriver;
    }

    return NULL;
}

static void
onControllerSerialNumberResponse(CanOpenPendingSdoRequest *request) {
    ConnectionAttempt *attempt;
    Node *node;

    attempt = (ConnectionAttempt *)request->context;
    node = &nodes[attempt->nodeId - 1];
    node->device = malloc(sizeof(*node->device));
    if (!node->device) {
        error("malloc failed for node device");
        pltExit(5);
    }
    node->device->driver = attempt->driver;

    if (createDevice(node->device, attempt->nodeId, request->response.data)) {
        free(node->device);
        node->device = NULL;
        free(attempt);
        return;
    }

    node->connected = veTrue;
    free(attempt);
}

static void onConnectionError(CanOpenPendingSdoRequest *request) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;

    free(attempt);
}

static void onControllerNameResponse(CanOpenPendingSdoRequest *request) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;

    attempt->driver = getDriverForNodeName(attempt->name, attempt->length);
    if (attempt->driver == NULL) {
        free(attempt);
        return; // Unsupported controller type
    }

    canOpenReadSdoAsync(attempt->nodeId, 0x1018, 4, attempt,
                        onControllerSerialNumberResponse, onConnectionError);
}

void connectToNode(un8 nodeId) {
    ConnectionAttempt *attempt;

    attempt = malloc(sizeof(*attempt));
    if (!attempt) {
        error("malloc failed for ConnectionAttempt");
        pltExit(5);
    }

    attempt->nodeId = nodeId;
    attempt->length = 0;
    attempt->driver = NULL;

    canOpenReadSegmentedSdoAsync(nodeId, 0x1008, 0, (void *)attempt,
                                 attempt->name, &attempt->length, 255,
                                 onControllerNameResponse, onConnectionError);
}

void disconnectFromNode(un8 nodeId) {
    Node *node;

    node = &nodes[nodeId - 1];
    if (!node->connected) {
        return;
    }
    destroyDevice(node->device);
    free(node->device);
    node->device = NULL;
    node->connected = veFalse;
}

void connectToDiscoveredNodes() {
    size_t i;
    un8 *ptr;
    un8 nodeId;
    Node *node;

    for (ptr = serviceManager.discoveredNodeIds.data, i = 0;
         i < serviceManager.discoveredNodeIds.count; i += 1, ptr += 1) {
        nodeId = *ptr;
        node = &nodes[nodeId - 1];
        if (!node->connected) {
            connectToNode(nodeId);
        }
    }
}

static void onReadRoutineComplete(CanOpenPendingSdoRequest *request) {
    Node *node;

    node = (Node *)request->context;
    if (!node->connected) {
        return;
    }

    veItemSendPendingChanges(node->device->root);
}

void readFromConnectedNodes() {
    un8 nodeId;
    Node *node;

    // @todo:
    // The queue still hasn't cleared up from the last read cycle,
    // Perhaps we will need a queue per node at some point
    if (canOpenState.pendingSdoRequests->first != NULL) {
        return;
    }

    for (nodeId = 1, node = nodes; nodeId <= 127; nodeId += 1, node += 1) {
        if (node->connected) {
            node->device->driver->readRoutine(node);
            canOpenQueueCallbackAsync(node, onReadRoutineComplete);
        }
    }
}

void nodesInit() { memset(nodes, 0, sizeof(nodes)); }