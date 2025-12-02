#include <discovery.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>

static Node nodes[127];

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

static void onControllerSerialNumberError(CanOpenPendingSdoRequest *request) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;
    free(attempt);
}

static void onDiscoverNodeSuccess(un8 nodeId, void *context, Driver *driver) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)context;
    attempt->driver = driver;

    canOpenReadSdoAsync(attempt->nodeId, 0x1018, 4, attempt,
                        onControllerSerialNumberResponse,
                        onControllerSerialNumberError);
}

static void onDiscoverNodeError(un8 nodeId, void *context) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)context;
    free(attempt);
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

    discoverNode(nodeId, onDiscoverNodeSuccess, onDiscoverNodeError,
                 (void *)attempt);
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

void readFromConnectedNodes(veBool fast) {
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
            if (fast == veTrue) {
                node->device->driver->fastReadRoutine(node);
            } else {
                node->device->driver->readRoutine(node);
            }
            canOpenQueueCallbackAsync(node, onReadRoutineComplete);
        }
    }
}

void nodesInit() { memset(nodes, 0, sizeof(nodes)); }