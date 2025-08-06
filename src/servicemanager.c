#include <canopen.h>
#include <localsettings.h>
#include <logger.h>
#include <servicemanager.h>
#include <stdio.h>
#include <string.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/types/ve_values.h>
#include <velib/utils/ve_item_utils.h>

ServiceManager serviceManager;

static struct VeSettingProperties discoveredNodesType = {
    .type = VE_STR,
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
        localSettings, "Settings/CanOpenMotordrive", serviceManager.root,
        "DiscoveredNodes", veVariantFmt, &veUnitNone, &discoveredNodesType);

    un8ArrayInit(&serviceManager.discoveredNodeIds);
    veItemSetChanged(serviceManager.discoveredNodes, onDiscoveredNodesChanged);

    veDbusItemInit(serviceManager.dbus, serviceManager.root);

    if (!veDbusChangeName(serviceManager.dbus,
                          "com.victronenergy.canopenmotordrive")) {
        error("could not register dbus service name "
              "'com.victronenergy.canopenmotordrive'");
        pltExit(3);
    }
}