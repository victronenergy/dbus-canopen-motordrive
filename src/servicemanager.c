#include <canopen.h>
#include <ctype.h>
#include <localsettings.h>
#include <logger.h>
#include <servicemanager.h>
#include <stdio.h>
#include <string.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/types/ve_values.h>
#include <velib/utils/ve_item_utils.h>

ServiceManager serviceManager;

static struct VeSettingProperties discoveredNodesType = {
    .type = VE_STR,
    .def.value.CPtr = "",
};

static un8 name[255];
static un8 length;

static void onScanEnd() {
    VeVariant v;

    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 100));
    un8ArraySerialize(&serviceManager.discoveredNodeIds,
                      serviceManager.discoveredNodes);
    veItemSet(serviceManager.scan, veVariantUn8(&v, 0));
    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 0));
}

static void onScanResponse(CanOpenPendingSdoRequest *request) {
    VeVariant v;

    if (getDriverForNodeName(name, length) != NULL) {
        un8ArrayAdd(&serviceManager.discoveredNodeIds, request->nodeId);
        // @todo: save discovered node name
    }

    if (request->nodeId % 13 == 0) {
        veItemSet(serviceManager.scanProgress,
                  veVariantUn8(&v, request->nodeId * 100 / 127));
    }

    if (request->nodeId == 127) {
        onScanEnd();
    }
}

static void onScanError(CanOpenPendingSdoRequest *request) {
    if (request->nodeId == 127) {
        onScanEnd();
    }
}

void scanBus() {
    VeVariant v;
    un8 nodeId;

    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 0));

    un8ArrayClear(&serviceManager.discoveredNodeIds);

    for (nodeId = 1; nodeId <= 127; nodeId += 1) {
        canOpenReadSegmentedSdoAsync(nodeId, 0x1008, 0, NULL, name, &length,
                                     255, onScanResponse, onScanError);
    }
}

static void onScanChanged(VeItem *item) {
    if (item->variant.value.UN8 == 1) {
        scanBus();
    }
}

static void onDiscoveredNodesChanged(VeItem *item) {
    un8ArrayDeserialize(&serviceManager.discoveredNodeIds, item);
    connectToDiscoveredNodes();
}

void serviceManagerInit(void) {
    VeVariant v;
    VeCanGateway *canGw;
    char canGwId[32];
    char *c;
    char serviceName[256];
    char settingsPrefix[256];

    canGw = veCanGwActive();
    veCanGwId(canGw, canGwId, sizeof(canGwId));
    for (c = canGwId; *c != 0; c += 1) {
        if (!isalnum(*c)) {
            *c = '_';
        }
    }

    snprintf(serviceName, sizeof(serviceName),
             "com.victronenergy.canopenmotordrive.%s", canGwId);
    snprintf(settingsPrefix, sizeof(settingsPrefix),
             "Settings/CanOpenMotordrive/%s", canGwId);

    serviceManager.dbus = veDbusConnectString(veDbusGetDefaultConnectString());
    if (!serviceManager.dbus) {
        error("dbus connection failed");
        pltExit(1);
    }

    serviceManager.root =
        veItemGetOrCreateUid(veValueTree(), "com.victonenergy.servicemanager");
    serviceManager.scan =
        veItemCreateBasic(serviceManager.root, "Scan", veVariantUn8(&v, 0));
    veItemSetChanged(serviceManager.scan, onScanChanged);
    serviceManager.scanProgress =
        veItemCreateQuantity(serviceManager.root, "ScanProgress",
                             veVariantUn8(&v, 0), &veUnitPercentage);
    serviceManager.discoveredNodes = veItemCreateSettingsProxy(
        localSettings, settingsPrefix, serviceManager.root, "DiscoveredNodes",
        veVariantFmt, &veUnitNone, &discoveredNodesType);

    un8ArrayInit(&serviceManager.discoveredNodeIds);
    veItemSetChanged(serviceManager.discoveredNodes, onDiscoveredNodesChanged);

    veDbusItemInit(serviceManager.dbus, serviceManager.root);

    if (!veDbusChangeName(serviceManager.dbus, serviceName)) {
        error("could not register dbus service name '%s'", serviceName);
        pltExit(3);
    }
}