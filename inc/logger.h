#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <velib/utils/ve_logger.h>

#define info(f, ...) logI("dbus-canopen-motordrive", f, ##__VA_ARGS__)
#define warning(f, ...) logW("dbus-canopen-motordrive", f, ##__VA_ARGS__)
#define error(f, ...) logE("dbus-canopen-motordrive", f, ##__VA_ARGS__)

#endif