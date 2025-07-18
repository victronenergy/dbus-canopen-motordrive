#include <canopen.h>
#include <curtis.h>
#include <device.h>
#include <localsettings.h>
#include <sevcon.h>
#include <stdlib.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

static un8 nodeId = 1;
static un16 task1sLastUpdate = 0;
static un16 task10sLastUpdate = 0;
static Device device;
static veBool connected = veFalse;

void taskEarlyInit(void) {
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void taskInit(void) {
    localSettingsInit();

    // @todo: get global settings for nodeId
}

void connectDevice() {
    un8 nodeName[255];
    un8 length;

    if (readSegmentedSdo(nodeId, 0x1008, 0, nodeName, &length, 255) != 0) {
        return;
    }
    if (length >= 4 && nodeName[0] == 'G' && nodeName[1] == 'e' &&
        nodeName[2] == 'n' && nodeName[3] == '4') {
        device.driver = &sevconDriver;
    } else if (length >= 4 && nodeName[0] == 'A' && nodeName[1] == 'C' &&
               nodeName[2] == ' ' && nodeName[3] == 'F') {
        device.driver = &curtisDriver;
    } else {
        return; // Unsupported controller type
    }

    if (createDevice(&device, nodeId) != 0) {
        return;
    }
    connected = veTrue;
}

void disconnectDevice() {
    destroyDevice(&device);
    connected = veFalse;
}

void task1s() {
    if (!connected) {
        return;
    }

    if (device.driver->readRoutine(&device)) {
        disconnectDevice();
        return;
    }
}

void task10s() {
    if (connected) {
        return;
    }

    connectDevice();
}

void taskUpdate(void) {
    VeRawCanMsg msg;

    while (veCanRead(&msg)) {
        // Prevents high CPU usage if nothing else consumes the CAN messages
    }

    if (veTick1ms(&task1sLastUpdate, 1000)) {
        task1s();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

void taskTick(void) {
    if (device.root) {
        veItemTick(device.root);
    }
}

char const *pltProgramVersion(void) { return VERSION; }