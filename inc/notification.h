#ifndef __NOTIFICATION_H__
#define __NOTIFICATION_H__

#include <velib/base/types.h>

typedef enum NotificationType {
    NOTIFICATION_TYPE_WARNING = 0,
    NOTIFICATION_TYPE_ERROR = 1,
    NOTIFICATION_TYPE_INFO = 2,
} NotificationType;

typedef struct _PendingNotification {
    un8 nodeId;
    NotificationType type;
    char *title;
    char *description;
    un16 timeout;
} PendingNotification;

void notificationsInit();
void queueNotification(un8 nodeId, NotificationType type, const char *title,
                       const char *description);
void processPendingNotifications();

#endif