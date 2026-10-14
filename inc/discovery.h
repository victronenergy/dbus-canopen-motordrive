#ifndef __DISCOVERY_H__
#define __DISCOVERY_H__

#include <driver.h>
#include <velib/types/ve_item_def.h>

#define CURTIS_VENDOR_ID 0x4349
#define DMC_VENDOR_ID 0x04F1
#define DMC_SIGMA2N_PRODUCT_CODE 0x22488014

typedef void (*DiscoverNodeSuccessCallback)(un8 nodeId, void *context,
                                            Driver *driver);
typedef void (*DiscoverNodeErrorCallback)(un8 nodeId, void *context);

typedef struct _DiscoveryContext {
    un8 nodeId;
    DiscoverNodeSuccessCallback onSuccess;
    DiscoverNodeErrorCallback onError;
    void *context;
} DiscoveryContext;

void discoverNode(un8 nodeId, DiscoverNodeSuccessCallback onSuccess,
                  DiscoverNodeErrorCallback, void *context);

#endif