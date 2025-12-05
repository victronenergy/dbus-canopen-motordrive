#include <canopen.h>
#include <ctype.h>
#include <discovery.h>
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

static void onScanEnd() {
    VeVariant v;

    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 100));
    un8ArraySerialize(&serviceManager.discoveredNodeIds,
                      serviceManager.discoveredNodes);
    veItemSet(serviceManager.scan, veVariantUn8(&v, 0));
    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 0));
}

static un8 scanNodeId;

static void updateScanProgress();

static void onDiscoverNodeSuccess(un8 nodeId, void *context, Driver *driver) {
    un8ArrayAdd(&serviceManager.discoveredNodeIds, nodeId);

    updateScanProgress();
}

static void onDiscoverNodeError(un8 nodeId, void *context) {
    updateScanProgress();
}

static void updateScanProgress() {
    VeVariant v;

    veItemSet(serviceManager.scanProgress,
              veVariantUn8(&v, scanNodeId * 100 / 127));

    scanNodeId += 1;

    if (scanNodeId <= 127) {
        discoverNode(scanNodeId, onDiscoverNodeSuccess, onDiscoverNodeError,
                     NULL);
    } else {
        onScanEnd();
    }
}

void scanBus() {
    VeVariant v;

    scanNodeId = 1;
    veItemSet(serviceManager.scanProgress, veVariantUn8(&v, 0));
    un8ArrayClear(&serviceManager.discoveredNodeIds);

    discoverNode(scanNodeId, onDiscoverNodeSuccess, onDiscoverNodeError, NULL);
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
    char canGwId[32];
    char serviceName[256];
    char settingsPrefix[256];

    pltCanGwId(canGwId, sizeof(canGwId));

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