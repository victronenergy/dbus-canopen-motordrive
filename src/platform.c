#include <logger.h>
#include <platform.h>
#include <stdio.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/types/ve_values.h>

void platformInit(void) {
    VeItem *item;
    VeVariant v;

    item = veItemGetOrCreateUid(veValueTree(), "com.victronenergy.platform");
    if (!veDbusAddRemoteService("com.victronenergy.platform", item, veTrue)) {
        warning("could not add remote service com.victronenergy.platform");
        return;
    }

    item = veItemGetOrCreateUid(item, "Notifications/Inject");
    veItemLocalSet(item, veVariantInvalidType(&v, VE_STR));
}

void injectPlatformNotification(NotificationType type, const char *title,
                                const char *description) {
    char payload[1024];
    VeVariant v;
    VeItem *item;

    item = veItemGetOrCreateUid(
        veValueTree(), "com.victronenergy.platform/Notifications/Inject");
    snprintf(payload, sizeof(payload), "%u\t%s\t%s", type, description, title);
    veVariantHeapStr(&v, payload);
    veItemSet(item, &v);
}