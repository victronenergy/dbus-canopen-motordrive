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
static VeVariantUnitFmt unitNewtonM0Dec = {0, "Nm"};

static void connectToDbus(Device *device) {
    device->dbus = veDbusConnectString(veDbusGetDefaultConnectString());
    if (!device->dbus) {
        error("dbus connection failed");
        pltExit(1);
    }
}

static void createDeviceIdentifier(Device *device) {
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
    snprintf(device->identifier, sizeof(device->identifier), "%s_%s_%u",
             canGwId, device->driver->name, device->serialNumber);
}

static void getVrmDeviceInstance(Device *device) {
    device->deviceInstance =
        veDbusGetVrmDeviceInstance(device->identifier, "motordrive", 99);
    if (device->deviceInstance < 0) {
        error("could not get device instance for %s. %d", device->identifier,
              device->deviceInstance);
        pltExit(2);
    }
}

static void registerDbusServiceName(Device *device) {
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

    device->root = veItemGetOrCreateUid(veValueTree(), device->identifier);

    veItemCreateBasic(device->root, "DeviceInstance",
                      veVariantUn32(&v, device->deviceInstance));
    veItemCreateBasic(device->root, "ProductId",
                      veVariantUn16(&v, device->driver->productId));
    veItemCreateBasic(
        device->root, "ProductName",
        veVariantStr(&v, veProductGetName(device->driver->productId)));

    veItemCreateBasic(device->root, "Connected", veVariantUn32(&v, 1));
    veItemCreateBasic(device->root, "Mgmt/Connection",
                      veVariantStr(&v, "VE.Can"));
    veItemCreateBasic(device->root, "Mgmt/ProcessName",
                      veVariantStr(&v, "dbus-canopen-motordrive"));
    veItemCreateBasic(device->root, "Mgmt/ProcessVersion",
                      veVariantStr(&v, pltProgramVersion()));

    snprintf(serialNumberStr, sizeof(serialNumberStr), "%u",
             device->serialNumber);
    veItemCreateBasic(device->root, "Serial",
                      veVariantHeapStr(&v, serialNumberStr));

    device->voltage =
        veItemCreateQuantity(device->root, "Dc/0/Voltage",
                             veVariantFloat(&v, 0.0F), &veUnitVolt2Dec);
    device->current =
        veItemCreateQuantity(device->root, "Dc/0/Current",
                             veVariantFloat(&v, 0.0F), &veUnitAmps1Dec);
    device->power = veItemCreateQuantity(device->root, "Dc/0/Power",
                                         veVariantSn32(&v, 0), &veUnitWatt);
    device->motorRpm =
        veItemCreateQuantity(device->root, "Motor/RPM",
                             veVariantInvalidType(&v, VE_UN16), &unitRpm0Dec);
    device->motorDirection = veItemCreateBasic(
        device->root, "Motor/Direction", veVariantInvalidType(&v, VE_UN8));
    device->motorTemperature = veItemCreateQuantity(
        device->root, "Motor/Temperature", veVariantInvalidType(&v, VE_UN16),
        &unitCelsius0Dec);
    device->motorTorque = veItemCreateQuantity(
        device->root, "Motor/Torque", veVariantInvalidType(&v, VE_UN16),
        &unitNewtonM0Dec);
    device->controllerTemperature = veItemCreateQuantity(
        device->root, "Controller/Temperature",
        veVariantInvalidType(&v, VE_UN16), &unitCelsius0Dec);

    snprintf(settingsPath, sizeof(settingsPath), "Settings/Devices/%s",
             device->identifier);
    device->motorDirectionInverted = veItemCreateSettingsProxyId(
        localSettings, settingsPath, device->root, "Motor/DirectionInverted",
        veVariantFmt, &veUnitNone, &booleanType,
        "Settings/Motor/DirectionInverted");

    veDbusItemInit(device->dbus, device->root);
}

veBool createDevice(Device *device, un8 nodeId, un32 serialNumber) {
    device->nodeId = nodeId;
    device->serialNumber = serialNumber;

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