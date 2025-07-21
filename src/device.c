#include <canopen.h>
#include <ctype.h>
#include <device.h>
#include <localsettings.h>
#include <logger.h>
#include <stdio.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_values.h>
#include <velib/utils/ve_item_utils.h>
#include <velib/vecan/products.h>

static struct VeSettingProperties booleanType = {
    .type = VE_SN32,
    .def.value.SN32 = 0,
    .min.value.SN32 = 0,
    .max.value.SN32 = 1,
};

static VeVariantUnitFmt unitRpm0Dec = {0, "RPM"};
static VeVariantUnitFmt unitCelsius0Dec = {0, "C"};

void connectToDbus(Device *device) {
    device->dbus = veDbusConnectString(veDbusGetDefaultConnectString());
    if (!device->dbus) {
        error("dbus connection failed");
        pltExit(1);
    }
}

void createDeviceIdentifier(Device *device) {
    VeCanGateway *canGw;
    char canGwId[32];
    char *c;

    canGw = veCanGwActive();
    veCanGwId(canGw, canGwId, sizeof(canGwId));
    for (c = canGwId; *c != 0; c += 1) {
        if (!isalnum(*c)) {
            *c = '_';
        }
    }
    snprintf(device->identifier, sizeof(device->identifier), "%s_%s_%d",
             canGwId, device->driver->name, device->serialNumber);
}

void getVrmDeviceInstance(Device *device) {
    device->deviceInstance =
        veDbusGetVrmDeviceInstance(device->identifier, "motordrive", 99);
    if (device->deviceInstance < 0) {
        error("could not get device instance. %d", device->deviceInstance);
        pltExit(2);
    }
}

void registerDbusServiceName(Device *device) {
    char serviceName[256];

    snprintf(serviceName, sizeof(serviceName),
             "com.victronenergy.motordrive.%s", device->identifier);
    if (!veDbusChangeName(device->dbus, serviceName)) {
        error("could not register dbus service name '%s'", serviceName);
        pltExit(3);
    }
}

void createDbusTree(Device *device) {
    VeVariant v;
    char serialNumberStr[11];
    char settingsPath[128];

    device->root =
        veItemGetOrCreateUid(veValueTree(), "com.victonenergy.producer");

    veItemCreateBasic(device->root, "DeviceInstance",
                      veVariantUn16(&v, device->deviceInstance));
    veItemCreateBasic(
        device->root, "ProductId",
        veVariantUn16(&v,
                      VE_PROD_ID_CITOLEN_SEVCON)); // @todo: do we want a
                                                   // generic product ID here?
    veItemCreateBasic(
        device->root, "ProductName",
        veVariantStr(&v, veProductGetName(VE_PROD_ID_CITOLEN_SEVCON)));

    veItemCreateBasic(device->root, "Connected", veVariantUn32(&v, 1));
    veItemCreateBasic(device->root, "Mgmt/Connection",
                      veVariantStr(&v, "VE.Can"));
    veItemCreateBasic(device->root, "Mgmt/ProcessName",
                      veVariantStr(&v, "dbus-sevcon"));
    veItemCreateBasic(device->root, "Mgmt/ProcessVersion",
                      veVariantStr(&v, pltProgramVersion()));

    snprintf(serialNumberStr, sizeof(serialNumberStr), "%d",
             device->serialNumber);
    veItemCreateBasic(device->root, "Serial",
                      veVariantHeapStr(&v, serialNumberStr));

    device->voltage =
        veItemCreateQuantity(device->root, "Dc/0/Voltage",
                             veVariantFloat(&v, 0.0F), &veUnitVolt2Dec);
    device->current =
        veItemCreateQuantity(device->root, "Dc/0/Current",
                             veVariantFloat(&v, 0.0F), &veUnitAmps1Dec);
    device->motorRpm =
        veItemCreateQuantity(device->root, "Motor/RPM",
                             veVariantInvalidType(&v, VE_UN16), &unitRpm0Dec);
    device->motorDirection = veItemCreateBasic(
        device->root, "Motor/Direction", veVariantInvalidType(&v, VE_UN8));
    device->motorTemperature = veItemCreateQuantity(
        device->root, "Motor/Temperature", veVariantInvalidType(&v, VE_UN16),
        &unitCelsius0Dec);
    device->controllerTemperature = veItemCreateQuantity(
        device->root, "Controller/Temperature",
        veVariantInvalidType(&v, VE_UN16), &unitCelsius0Dec);

    snprintf(settingsPath, sizeof(settingsPath), "Settings/Devices/%s",
             device->identifier);
    device->motorDirectionInverted =
        veItemCreateSettingsProxy(localSettings, settingsPath, device->root,
                                  "Motor/DirectionInverted",
                                  veVariantFmt, &veUnitNone, &booleanType);

    if (device->driver->onBeforeDbusInit) {
        device->driver->onBeforeDbusInit(device);
    }

    veDbusItemInit(device->dbus, device->root);
}

veBool createDevice(Device *device, un8 nodeId) {
    if (device->driver->getSerialNumber(nodeId, &device->serialNumber)) {
        return veTrue;
    }

    device->nodeId = nodeId;
    connectToDbus(device);
    createDeviceIdentifier(device);
    getVrmDeviceInstance(device);
    createDbusTree(device);
    registerDbusServiceName(device);

    return veFalse;
}

void destroyDevice(Device *device) {
    veDbusDisconnect(device->dbus);
    device->dbus = NULL;
    veItemDeleteBranch(device->root);
    device->root = NULL;

    device->nodeId = 0;
    device->deviceInstance = 0;
    device->serialNumber = 0;
}