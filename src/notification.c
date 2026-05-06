#include <list.h>
#include <logger.h>
#include <memory.h>
#include <node.h>
#include <notification.h>
#include <platform.h>
#include <string.h>
#include <velib/utils/ve_timer.h>

/**
 * The Curtis E/SE controller tends to trigger EMCY messages right as it is
 * shutting down. To avoid showing notifications for these messages, we will
 * delay the injection of notifications by 1 second, and check if the node is
 * still connected before injecting the notification.
 */
#define NOTIFICATION_INJECTION_DELAY_MS 1000

static List *pendingNotifications;

void notificationsInit() { pendingNotifications = listCreate(); }

void queueNotification(un8 nodeId, NotificationType type, const char *title,
                       const char *description) {
    PendingNotification *notification;

    notification = _malloc(sizeof(PendingNotification));
    CHECK_ALLOC(notification);

    notification->nodeId = nodeId;
    notification->type = type;
    notification->title = _strdup(title);
    CHECK_ALLOC(notification->title);
    notification->description = _strdup(description);
    CHECK_ALLOC(notification->description);
    notification->timeout = pltGetCount1ms();

    listAdd(pendingNotifications, notification);
}

void processPendingNotifications() {
    ListItem *item = pendingNotifications->first;
    while (item) {
        ListItem *next = item->next;
        PendingNotification *notification = (PendingNotification *)item->data;
        if (veTick1ms(&notification->timeout,
                      NOTIFICATION_INJECTION_DELAY_MS)) {

            if (isNodeConnected(notification->nodeId)) {
                injectPlatformNotification(notification->type,
                                           notification->title,
                                           notification->description);
            } else {
                warning("Ignoring notification for node %u since it is not "
                        "connected",
                        notification->nodeId);
            }

            _free(notification->title);
            _free(notification->description);
            _free(notification);
            listRemove(pendingNotifications, item);
        }
        item = next;
    }
}
