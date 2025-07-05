#include <ctype.h>
#include <device.h>
#include <logger.h>
#include <stdio.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_values.h>
#include <velib/utils/ve_item_utils.h>
#include <velib/vecan/products.h>

static VeVariantUnitFmt unitVoltage2Dec = {2, "V"};
static VeVariantUnitFmt unitAmps1Dec = {1, "A"};
static VeVariantUnitFmt unitRpm0Dec = {0, "RPM"};
static VeVariantUnitFmt unitCelsius0Dec = {0, "C"};

void connectToDbus(Device *device) {
    device->dbus = veDbusGetDefaultBus();
    if (!device->dbus) {
        error("dbus connection failed");
        pltExit(1);
    }
    veDbusSetListeningDbus(device->dbus);
}

void getVrmDeviceInstance(Device *device) {
    char identifier[23];

    snprintf(identifier, sizeof(identifier), "dbus_sevcon_%d",
             device->serialNumber);
    device->deviceInstance =
        veDbusGetVrmDeviceInstance(identifier, "motordrive", 99);
    if (device->deviceInstance < 0) {
        error("could not get device instance. %d", device->deviceInstance);
        pltExit(2);
    }
}

void registerDbusServiceName(Device *device) {
    char serviceName[256];
    char canGwId[32];
    char *c;

    VeCanGateway *canGw = veCanGwActive();
    veCanGwId(canGw, canGwId, sizeof(canGwId));
    for (c = canGwId; *c != 0; c += 1) {
        if (!isalnum(*c)) {
            *c = '_';
        }
    }
    snprintf(serviceName, sizeof(serviceName),
             "com.victronenergy.motordrive.%s", canGwId);
    if (!veDbusChangeName(device->dbus, serviceName)) {
        error("could not register dbus service name '%s'", serviceName);
        pltExit(3);
    }
}

void createDbusTree(Device *device) {
    VeVariant v;

    device->root =
        veItemGetOrCreateUid(veValueTree(), "com.victonenergy.producer");

    veItemCreateBasic(device->root, "DeviceInstance",
                      veVariantUn16(&v, device->deviceInstance));
    veItemCreateBasic(device->root, "ProductId",
                      veVariantUn16(&v, VE_PROD_ID_CITOLEN_SEVCON));
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

    veItemAddQuantityAndProduce(device->root, "Dc/0/Voltage", &device->voltage,
                                veVariantInvalidType(&v, VE_FLOAT),
                                &unitVoltage2Dec);
    veItemAddQuantityAndProduce(device->root, "Dc/0/Current", &device->current,
                                veVariantInvalidType(&v, VE_FLOAT),
                                &unitAmps1Dec);
    veItemAddQuantityAndProduce(device->root, "Motor/RPM", &device->rpm,
                                veVariantInvalidType(&v, VE_UN16),
                                &unitRpm0Dec);
    veItemAddBasicAndProduce(device->root, "Motor/Direction",
                             &device->direction,
                             veVariantInvalidType(&v, VE_UN8));
    veItemAddQuantityAndProduce(
        device->root, "Motor/Temperature", &device->temperature,
        veVariantInvalidType(&v, VE_UN16), &unitCelsius0Dec);

    veDbusItemInit(device->dbus, device->root);
}

void createDevice(Device *device, un32 serialNumber) {
    device->serialNumber = serialNumber;

    connectToDbus(device);
    getVrmDeviceInstance(device);
    createDbusTree(device);
    registerDbusServiceName(device);
}

void destroyDevice(Device *device) {
    veDbusDisconnect(device->dbus);
    device->dbus = NULL;
    veItemDeleteBranch(device->root);
    device->root = NULL;

    device->deviceInstance = 0;
    device->serialNumber = 0;
}