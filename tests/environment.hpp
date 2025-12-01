#pragma once

#include "fff.h"

extern "C" {
#include <velib/base/types.h>
#include <velib/canhw/canhw_driver.h>
#include <velib/types/ve_item_def.h>
}

extern "C" {
DECLARE_FAKE_VALUE_FUNC1(void *, _malloc, size_t);
DECLARE_FAKE_VALUE_FUNC2(void *, _realloc, void *, size_t);
DECLARE_FAKE_VOID_FUNC1(_free, void *);

DECLARE_FAKE_VALUE_FUNC2(veBool, veItemSet, VeItem *, VeVariant *);
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
}