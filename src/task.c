#include <canopen.h>
#include <curtis.h>
#include <device.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <sevcon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

static un16 task1sLastUpdate = 0;
static un16 task10sLastUpdate = 0;

void taskEarlyInit(void) {
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void task10s() {
    if (serviceManager.scan->variant.value.UN8 == 1) {
        return;
    }

    connectToDiscoveredNodes();
}

void taskInit(void) {
    task1sLastUpdate = pltGetCount1ms();
    task10sLastUpdate = pltGetCount1ms();

    nodesInit();
    canOpenInit();
    localSettingsInit();
    serviceManagerInit();

    task10s();
}

void task1s() {
    if (serviceManager.scan->variant.value.UN8 == 1) {
        return;
    }

    readFromConnectedNodes();
}

void taskUpdate(void) {
    canOpenTx();
    canOpenRx();
}

void taskTick(void) {
    nodesTick();
    veItemTick(serviceManager.root);

    if (veTick1ms(&task1sLastUpdate, 1000)) {
        task1s();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

char const *pltProgramVersion(void) { return VERSION; }