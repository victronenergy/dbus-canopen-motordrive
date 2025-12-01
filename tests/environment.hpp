#pragma once

// #define FFF_GCC_FUNCTION_ATTRIBUTES __attribute__((weak))
#include "fff.h"

extern "C" {
#include <dbus/dbus.h>
#include <ve_dbus_internal.h>
#include <velib/base/types.h>
#include <velib/base/ve_string.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/types/ve_item_def.h>
}

extern "C" {
DECLARE_FAKE_VALUE_FUNC1(void *, _malloc, size_t);
DECLARE_FAKE_VALUE_FUNC2(void *, _realloc, void *, size_t);
DECLARE_FAKE_VOID_FUNC1(_free, void *);

DECLARE_FAKE_VALUE_FUNC1(struct VeDbus *, veDbusConnectString, char const *);
DECLARE_FAKE_VALUE_FUNC0(struct VeDbus *, veDbusGetDefaultBus);
DECLARE_FAKE_VALUE_FUNC5(sn32, veDbusGetVrmDeviceInstanceExt, char const *,
                         char const *, sn32, VeVariant *, veBool);
DECLARE_FAKE_VALUE_FUNC2(veBool, veDbusChangeName, struct VeDbus *,
                         char const *);
DECLARE_FAKE_VALUE_FUNC1(veBool, veCanSend, VeRawCanMsg *);
DECLARE_FAKE_VALUE_FUNC1(veBool, veCanRead, VeRawCanMsg *);
DECLARE_FAKE_VALUE_FUNC0(un16, pltGetCount1ms);
DECLARE_FAKE_VALUE_FUNC3(struct VeRemoteService *, veDbusAddRemoteService,
                         char const *, struct VeItem *, veBool);
}

extern "C" {
char const *pltProgramVersion(void);
void pltWatchFileDescriptor(int fd);
void taskEarlyInit(void);
void taskInit(void);
veBool veCanBusOn(void);
void veCanDrvEnumerate(void);
VeCanDriver *veCanDrvGet(const char *id);
un8 veCanDrvGetGwCount(void);
void veCanDrvPrintGwList(void);
VeCanGateway *veCanDrvProbeGw(VeCanDriver *driver, char const *gwId);
size_t veCanGwDesc(VeCanGateway *gw, char *buf, size_t len);
extern VeCanGateway *veCanGwFindDefault(void);
VeCanGateway *veCanGwGet(char const *id);
VeCanRxEventType veCanGwRxEventType(VeCanGateway *gw, VeCanRxEventData *data);
void veCanGwSetRxEventCb(VeCanGateway *gw, VeCanGwRxEventCb *cb);
void veCanInit(void);
veBool veCanOpen(void);
veBool veCanSetBitRate(un16 kbit);
un8 veCanShowTrace(un8 dump);
void veTodo(void);

char const *veDbusGetDefaultConnectString(void);
void veDbusSetListeningDbus(struct VeDbus *dbus);
struct VeRemoteService *veDbusAddRemoteService(char const *serviceName,
                                               struct VeItem *dbusRoot,
                                               veBool block);
VeCanGateway *veCanGwActive(void);
size_t veCanGwId(VeCanGateway *gw, char *buf, size_t len);
struct VeItem *veValueTree(void);
void veDbusItemInit(VeDbus *dbus, struct VeItem *items);
void veDbusDisconnect(VeDbus *dbus);
void veDbusItemTick(void);
veBool veDBusAddLocalSetting(struct VeItem *item, VeVariant *defaultValue,
                             VeVariant *minValue, VeVariant *maxValue,
                             veBool hang);
}