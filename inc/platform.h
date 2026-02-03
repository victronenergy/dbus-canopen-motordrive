#ifndef __PLATFORM_H__
#define __PLATFORM_H__

typedef enum NotificationType {
    NOTIFICATION_TYPE_WARNING = 0,
    NOTIFICATION_TYPE_ERROR = 1,
    NOTIFICATION_TYPE_INFO = 2,
} NotificationType;

void platformInit(void);

void injectPlatformNotification(NotificationType type, const char *title,
                                const char *description);

#endif