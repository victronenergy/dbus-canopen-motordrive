#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <velib/utils/ve_logger.h>

#define info(f, ...) logI("dbus-sevcon", f, ##__VA_ARGS__)
#define error(f, ...) logE("dbus-sevcon", f, ##__VA_ARGS__)

#endif