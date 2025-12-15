#include <canopen.h>
#include <discovery.h>
#include <drivers/curtis_e.h>
#include <drivers/curtis_f.h>
#include <drivers/sevcon.h>
#include <logger.h>
#include <memory.h>
#include <velib/platform/plt.h>

static un8 name[255];
static un8 length;

static Driver *getDriverForNodeName(un8 *name, un8 length) {
    if (length >= 4 && name[0] == 'G' && name[1] == 'e' && name[2] == 'n' &&
        name[3] == '4') {
        return &sevconDriver;
    } else if (length >= 4 && name[0] == 'A' && name[1] == 'C' &&
               name[2] == ' ' && name[3] == 'F') {
        return &curtisFDriver;
    }
    // Curtis 123X SE/E controllers do not support SDO 0x1008

    return NULL;
}

static void onSuccess(DiscoveryContext *context, Driver *driver) {
    context->onSuccess(context->nodeId, context->context, driver);
    _free(context);
}

static void onError(DiscoveryContext *context) {
    context->onError(context->nodeId, context->context);
    _free(context);
}

static void onCurtisModelNumberResponse(CanOpenPendingSdoRequest *request) {
    un32 modelNumber = request->response.data;

    if ((modelNumber >= 12320000 && modelNumber <= 12329999) ||
        (modelNumber >= 12340000 && modelNumber <= 12349999) ||
        (modelNumber >= 12360000 && modelNumber <= 12369999) ||
        (modelNumber >= 12380000 && modelNumber <= 12389999)) {
        onSuccess((DiscoveryContext *)request->context, &curtisEDriver);
    } else {
        onError((DiscoveryContext *)request->context);
    }
}

static void onCurtisModelNumberError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    onError((DiscoveryContext *)request->context);
}

static void onVendorIdResponse(CanOpenPendingSdoRequest *request) {
    if (request->response.data == CURTIS_VENDOR_ID) {
        canOpenReadSdoAsync(request->nodeId, 0x3464, 0, request->context,
                            onCurtisModelNumberResponse,
                            onCurtisModelNumberError);
    } else {
        onError((DiscoveryContext *)request->context);
    }
}

static void onVendorIdError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    onError((DiscoveryContext *)request->context);
}

static void onProductNameSuccess(CanOpenPendingSdoRequest *request) {
    DiscoveryContext *context;
    Driver *driver;

    context = (DiscoveryContext *)request->context;
    driver = getDriverForNodeName(name, length);
    if (driver != NULL) {
        onSuccess(context, driver);
    } else {
        onError(context);
    }
}

static void onProductNameError(CanOpenPendingSdoRequest *request, CanOpenError error) {
    // Some controllers do not support reading the product name. (e.g. Curtis
    // 123X SE/E). Falling back to reading the vendor ID.
    if (error != SDO_READ_ERROR_TIMEOUT) {
        canOpenReadSdoAsync(request->nodeId, 0x1018, 1, request->context,
                            onVendorIdResponse, onVendorIdError);
    } else {
        onError((DiscoveryContext *)request->context);
    }
}

void discoverNode(un8 nodeId, DiscoverNodeSuccessCallback onSuccess,
                  DiscoverNodeErrorCallback onError, void *context) {
    DiscoveryContext *discoveryContext;

    discoveryContext = _malloc(sizeof(*discoveryContext));
    if (!discoveryContext) {
        error("malloc failed for discovery context"); // @todo
        pltExit(5);
    }

    discoveryContext->nodeId = nodeId;
    discoveryContext->onSuccess = onSuccess;
    discoveryContext->onError = onError;
    discoveryContext->context = context;

    canOpenReadSegmentedSdoAsync(nodeId, 0x1008, 0, discoveryContext, name,
                                 &length, 255, onProductNameSuccess,
                                 onProductNameError);
}