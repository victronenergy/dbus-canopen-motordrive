#include "environment.hpp"

DEFINE_FFF_GLOBALS;

extern "C" {
DEFINE_FAKE_VALUE_FUNC1(void *, _malloc, size_t);
DEFINE_FAKE_VALUE_FUNC2(void *, _realloc, void *, size_t);
DEFINE_FAKE_VOID_FUNC1(_free, void *);

DEFINE_FAKE_VALUE_FUNC2(veBool, veItemSet, VeItem *, VeVariant *);
}

extern "C" {

char const *pltProgramVersion(void) { return "test_version"; }
void pltWatchFileDescriptor(int fd) {}
void taskEarlyInit(void) {}
void taskInit(void) {}
veBool veCanBusOn(void) { return veTrue; }
void veCanDrvEnumerate(void) {}
VeCanDriver *veCanDrvGet(const char *id) { return NULL; }
un8 veCanDrvGetGwCount(void) { return 0; }
void veCanDrvPrintGwList(void) {}
VeCanGateway *veCanDrvProbeGw(VeCanDriver *driver, char const *gwId) {
    return NULL;
}
size_t veCanGwDesc(VeCanGateway *gw, char *buf, size_t len) { return 0; }
VeCanGateway *veCanGwFindDefault(void) { return NULL; }
VeCanGateway *veCanGwGet(char const *id) { return NULL; }
VeCanRxEventType veCanGwRxEventType(VeCanGateway *gw, VeCanRxEventData *data) {
    return VE_CAN_RX_EV_ASYNC_CALLBACK;
}
void veCanGwSetRxEventCb(VeCanGateway *gw, VeCanGwRxEventCb *cb) {}
void veCanInit(void) {}
veBool veCanOpen(void) { return veTrue; }
veBool veCanSetBitRate(un16 kbit) { return veTrue; }
un8 veCanShowTrace(un8 dump) { return 0; }
void veTodo(void) {}
VeVariant* veItemLocalValue(VeItem *item, VeVariant *var) {
    *var = item->variant;
    return var;
}
}