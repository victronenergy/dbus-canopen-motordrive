#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <notification.h>

void injectPlatformNotification(NotificationType type, const char *title,
                                const char *description);

#endif