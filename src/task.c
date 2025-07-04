#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <velib/canhw/canhw.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_values.h>
#include <velib/types/ve_item_def.h>
#include <velib/platform/plt.h>
#include <velib/utils/ve_logger.h>
#include <velib/utils/ve_timer.h>
#include <velib/utils/ve_item_utils.h>
#include <velib/vecan/products.h>
#include <canopen.h>

// CANOpen node ID for the Sevcon controller.
un8 nodeId = 1;

// 1008h - Controller name.
// 1009h - Hardware version.
// 100Ah - Software version.
// 1018h - Identity object. Contains CANopen vendor ID, product code, CANopen protocol revision, and controller serial number.

// veDbusGetDefaultBus - This will connect to the default dbus. If the connection fails, it will return NULL.
// veDbusDisconnect - This will disconnect from the dbus and free resources
// veDbusChangeName - This will change the service name on the dbus for the connection. Return veTrue on success.
// veDbusItemInit - Register an item tree on the dbus

// VeVariant - Represents data.

// veDbusGetVrmDeviceInstance identifier="/Settings/Devices/dbusSevcon_<serial from controller>"" deviceClass="motordrive" preferredInstance=99 - Obtain a device instance from the dbus. (VRM)

// dbusservice.add_path('/DeviceInstance', value=99) - unsigned 16
// dbusservice.add_path('/ProductId', value=0xC06C) - unsigned 16 VE_PROD_ID_CITOLEN_SEVCON
// dbusservice.add_path('/ProductName', value='Sevcon Gen4') - string veProductGetName(VE_PROD_ID_GENERIC_BATTERY)
// dbusservice.add_path('/FirmwareVersion', value='0.0.0')
// dbusservice.add_path('/HardwareVersion', value='0.0.0')
// dbusservice.add_path('/Connected', value=1)
// dbusservice.add_path('/Motor/RPM', value=None)
// dbusservice.add_path('/Motor/Temperature', value=None)
// dbusservice.add_path('/Motor/Direction', value=None)
// dbusservice.add_path('/Dc/0/Voltage', value=None)
// dbusservice.add_path('/Dc/0/Current', value=None)
// dbusservice.add_path('/Dc/0/Temperature', value=None)
// dbusservice.add_path('/Mgmt/Connection', value='VE.Can')
// dbusservice.add_path('/Mgmt/ProcessName', value='dbus_sevcon')
// dbusservice.add_path('/Mgmt/ProcessVersion', value='v0.0.0')

// VeRawCanMsg msg;
// veCanSend(&msg);

struct VeDbus *dbus;
// static VeVariantUnitFmt unitVoltage1Dec = {1, "V"};
static VeVariantUnitFmt unitVoltage2Dec = {2, "V"};
// static VeVariantUnitFmt unitVoltage3Dec = {3, "V"};
static VeVariantUnitFmt unitAmps1Dec = {1, "A"};
static VeVariantUnitFmt unitRpm0Dec = {0, "RPM"};

typedef struct
{
    VeItem voltage;
    VeItem current;
} Items;

static Items items;

void taskEarlyInit(void)
{
#ifdef CFG_WITH_CANHW_SOCKETCAN
    {
        VeCanDriver *drv = veCanSkRegister();
        if (drv)
        {
            veCanDrvRegister(drv);
        }
    }
#endif
    puts("Task early initialized");
}

uint32_t login()
{
    SdoMessage response;

    if (readSdo(nodeId, 0x5000, 1, &response) != 0)
    {
        return 1;
    }

    if (response.data != 4)
    {
        // set user id
        if (writeSdo(nodeId, 0x5000, 3, 0) != 0)
        {
            return 1;
        }
        // set password
        if (writeSdo(nodeId, 0x5000, 2, 0x4bdf) != 0)
        {
            return 1;
        }

        if (readSdo(nodeId, 0x5000, 1, &response) != 0)
        {
            return 1;
        }
        if (response.data != 4)
        {
            return 1;
        }
    }

    return 0;
}

void fetchBatteryVoltage()
{
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 1, &response) != 0)
    {
        printf("Could not read battery voltage\n");
    }
    float voltage = response.data * 0.0625F;

    printf("Battery voltage: %f\n", voltage);
}

void fetchBatteryCurrent()
{
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 2, &response) != 0)
    {
        printf("Could not read battery current\n");
    }
    float current = response.data * 0.0625F;

    printf("Battery current: %f\n", current);
}

void fetchEngineRpm()
{
    SdoMessage response;
    if (readSdo(nodeId, 0x606c, 0, &response) != 0)
    {
        printf("Could not read engine rpm\n");
    }
    int32_t rpm = response.data;

    printf("Rpm: %d\n", rpm);
}

void fetchEngineTemperature()
{
    SdoMessage response;
    if (readSdo(nodeId, 0x4600, 3, &response) != 0)
    {
        printf("Could not read engine temperature\n");
    }
    int32_t temperature = response.data;

    printf("Temperature: %d\n", temperature);
}

void taskInit(void)
{
    veLogLevel(3);
    // char serviceName[256];
    // char buffer[32];
    // char *c;

    // logI("dbus-sevcon", "Task initialized");
    // dbus = veDbusGetDefaultBus();
    // veDbusSetListeningDbus(dbus);

    // sn32 result = veDbusGetVrmDeviceInstance("dbus_sevcon_000000", "motordrive", 99);
    // printf("VRM instance number: %d\n", result);

    // strcpy(serviceName, "com.victronenergy.motordrive.");
    // VeCanGateway *canGw = veCanGwActive();
    // veCanGwId(canGw, buffer, sizeof(buffer));
    // for (c = buffer; *c != 0; c += 1) {
    //     if (!isalnum(*c)) {
    //         *c= '_';
    //     }
    // }
    // strcat(serviceName, buffer);
    // printf("Service name: %s\n", serviceName);

    // struct VeItem *root = veItemGetOrCreateUid(veValueTree(), "com.victonenergy.producer");

    // VeVariant v;
    // veItemCreateBasic(root, "DeviceInstance", veVariantUn16(&v, result));
    // veItemCreateBasic(root, "ProductId", veVariantUn16(&v, VE_PROD_ID_CITOLEN_SEVCON));
    // veItemCreateBasic(root, "ProductName", veVariantStr(&v, veProductGetName(VE_PROD_ID_CITOLEN_SEVCON)));

    // veItemCreateBasic(root, "Connected", veVariantUn32(&v, 1));
    // veItemCreateBasic(root, "Mgmt/Connection", veVariantStr(&v, "CAN-bus"));
    // veItemCreateBasic(root, "Mgmt/ProcessName", veVariantStr(&v, "dbus-sevcon"));
    // veItemCreateBasic(root, "Mgmt/ProcessVersion", veVariantStr(&v, pltProgramVersion()));

    // struct VeItem *energy = veItemGetOrCreateUid(root, "Dc/0");
    // veItemAddQuantityAndProduce(energy, "Voltage", &items.voltage, veVariantFloat(&v, 53.9F), &unitVoltage2Dec);
    // veItemAddQuantityAndProduce(energy, "Current", &items.current, veVariantFloat(&v, 10.0f), &unitAmps1Dec);

    // veItemCreateQuantity(root, "Motor/RPM", veVariantUn16(&v, 234), &unitRpm0Dec);
    // veItemCreateBasic(root, "Motor/Direction", veVariantUn8(&v, 2));

    // veDbusItemInit(dbus, root);

    // if (!veDbusChangeName(dbus, serviceName)) {
    // 	logE("dbus-sevcon", "dbus_service: registering name failed");
    // 	pltExit(11);
    // }

    // printf("login result: %d\n", login());

    // SdoMessage response;
    // printf("Read result: %d\n", readSdo(0x1018, 1, &response));
    // logSdoBuffer(&response);
    // printf("Read result: %d\n", readSdo(0x1018, 2, &response));
    // logSdoBuffer(&response);
    // printf("Read result: %d\n", readSdo(0x1018, 3, &response));
    // logSdoBuffer(&response);
    // printf("Read result: %d\n", readSdo(0x1018, 4, &response));
    // logSdoBuffer(&response);
    // printf("Serial number: %d\n", response.data);

    // fetchBatteryVoltage();
    // fetchBatteryCurrent();
    // fetchEngineRpm();
    // fetchEngineTemperature();

    // SdoMessage response;
    // printf("Read result: %d\n", readSdo(0x5000, 1, &response));
    // printf("Login level: %d\n", response.data);
}

void taskUpdate(void)
{
    // puts("Task updated");
    // VeRawCanMsg msg;

    // while (veCanRead(&msg))
    // {
    //     // printf("New message: %d\n", msg.canId);
    //     logRawCanMessage(&msg);
    // }
}

void taskTick(void)
{
    // puts("Task ticked");
}

char const *pltProgramVersion(void)
{
    return "v0.0";
}

// @todo:
// - every 10s check if the engine is ON and available on can bus
// - every 1s pull values from controller and publish them on dbus
// - on connection, connect to dbus and create device tree
// - on disconnection, disconnect from dbus and clear device