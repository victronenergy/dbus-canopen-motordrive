#include <canopen.h>
#include <localsettings.h>
#include <logger.h>
#include <node.h>
#include <servicemanager.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/utils/ve_timer.h>

#define TASK_FAST_READ_DELAY_MS 125
#define TASK_READ_DELAY_MS 1000
#define TASK_CONNECT_DELAY_MS 10000

static un16 taskFastReadLastUpdate = 0;
static un16 taskReadLastUpdate = 0;
static un16 taskConnectLastUpdate = 0;

void taskEarlyInit(void) {
    VeCanDriver *drv = veCanSkRegister();
    if (drv) {
        veCanDrvRegister(drv);
    }
}

void taskConnect() {
    if (serviceManager.scan->variant.value.UN8 == 1) {
        return;
    }

    connectToDiscoveredNodes();
}

void taskInit(void) {
    taskFastReadLastUpdate = pltGetCount1ms();
    taskReadLastUpdate = pltGetCount1ms();
    taskConnectLastUpdate = pltGetCount1ms();

    nodesInit();
    canOpenInit();
    localSettingsInit();
    serviceManagerInit();

    taskConnect();
}

void taskRead() {
    if (serviceManager.scan->variant.value.UN8 == 1) {
        return;
    }

    readFromConnectedNodes(veFalse);
}

void taskFastRead() {
    if (serviceManager.scan->variant.value.UN8 == 1) {
        return;
    }

    readFromConnectedNodes(veTrue);
}

void taskUpdate(void) {
    canOpenTx();
    canOpenRx();
}

void taskTick(void) {
    veItemTick(serviceManager.root);

    if (veTick1ms(&taskFastReadLastUpdate, TASK_FAST_READ_DELAY_MS)) {
        if (veTick1ms(&taskReadLastUpdate, TASK_READ_DELAY_MS)) {
            taskRead();
        } else {
            taskFastRead();
        }
    }

    if (veTick1ms(&taskConnectLastUpdate, TASK_CONNECT_DELAY_MS)) {
        taskConnect();
    }
}

char const *pltProgramVersion(void) { return VERSION; }