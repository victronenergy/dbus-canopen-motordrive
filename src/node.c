#include <discovery.h>
#include <logger.h>
#include <memory.h>
#include <node.h>
#include <servicemanager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/platform/plt.h>

Node nodes[127];

static void createNodeFromSerialNumber(ConnectionAttempt *attempt,
                                       const char *serialNumber) {
    Node *node;

    node = &nodes[attempt->nodeId - 1];
    node->device = _malloc(sizeof(*node->device));
    if (!node->device) {
        error("malloc failed for node device");
        pltExit(5);
    }
    node->device->driver = attempt->driver;

    createDevice(node->device, attempt->nodeId, serialNumber);
    node->connected = veTrue;
    if (node->device->driver->createDriverContext != NULL) {
        node->device->driverContext =
            node->device->driver->createDriverContext(node);
    }
    _free(attempt);
}

static void onCustomSerialNumberResponse(FetchSerialNumberRequest *request,
                                         const char *serialNumber) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;
    createNodeFromSerialNumber(attempt, serialNumber);

    _free(request);
}

static void onCustomSerialNumberError(FetchSerialNumberRequest *request) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;
    _free(attempt);
    _free(request);
}

static void onStandardSerialNumberResponse(CanOpenPendingSdoRequest *request) {
    ConnectionAttempt *attempt;
    char serialNumber[11];

    attempt = (ConnectionAttempt *)request->context;
    snprintf(serialNumber, sizeof(serialNumber), "%u", request->response.data);

    createNodeFromSerialNumber(attempt, serialNumber);
}

static void onStandardSerialNumberError(CanOpenPendingSdoRequest *request,
                                        CanOpenError error) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)request->context;
    _free(attempt);
}

static void onDiscoverNodeSuccess(un8 nodeId, void *context, Driver *driver) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)context;
    attempt->driver = driver;

    if (driver->fetchSerialNumber != NULL) {
        FetchSerialNumberRequest *request;

        request = _malloc(sizeof(*request));
        if (!request) {
            error("malloc failed for FetchSerialNumberRequest");
            pltExit(5);
        }
        request->nodeId = nodeId;
        request->context = (void *)attempt;
        request->onSuccess = onCustomSerialNumberResponse;
        request->onError = onCustomSerialNumberError;
        driver->fetchSerialNumber(request);
    } else {
        canOpenReadSdoAsync(attempt->nodeId, 0x1018, 4, attempt,
                            onStandardSerialNumberResponse,
                            onStandardSerialNumberError);
    }
}

static void onDiscoverNodeError(un8 nodeId, void *context) {
    ConnectionAttempt *attempt;

    attempt = (ConnectionAttempt *)context;
    _free(attempt);
}

void connectToNode(un8 nodeId) {
    ConnectionAttempt *attempt;

    attempt = _malloc(sizeof(*attempt));
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
    if (node->device->driver->destroyDriverContext != NULL) {
        node->device->driver->destroyDriverContext(node,
                                                   node->device->driverContext);
    }
    _free(node->device);
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

void nodesEmcyHandler(void *context, un8 nodeId, VeRawCanMsg *message) {
    Node *node;

    node = &nodes[nodeId - 1];
    if (node->connected) {
        if (node->device->driver->onEMCYMessage != NULL) {
            node->device->driver->onEMCYMessage(node, message);
        }
    }
    warning("Unhandled EMCY message from node %u", nodeId);
}

void nodesInit() { memset(nodes, 0, sizeof(nodes)); }