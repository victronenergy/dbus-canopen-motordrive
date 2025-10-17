#include <canopen.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

static un16 task500msLastUpdate = 0;
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
    task500msLastUpdate = pltGetCount1ms();
    task10sLastUpdate = pltGetCount1ms();

    nodesInit();
    canOpenInit();
    localSettingsInit();
    serviceManagerInit();

    task10s();
}

void task500ms() {
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
    veItemTick(serviceManager.root);

    if (veTick1ms(&task500msLastUpdate, 500)) {
        task500ms();
    }
    if (veTick1ms(&task10sLastUpdate, 10000)) {
        task10s();
    }
}

char const *pltProgramVersion(void) { return VERSION; }