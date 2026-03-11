#include <dbus/dbus.h>
#include <notification.h>
#include <ve_dbus_internal.h>
#include <velib/types/ve_dbus_item.h>

void injectPlatformNotification(NotificationType type, const char *title,
                                const char *description) {
    DBusMessage *msg;
    DBusMessageIter itt;
    char payload[1024];
    VeVariant v;

    struct VeDbus *dbus = veDbusGetDefaultBus();
    if (dbus == NULL) {
        return;
    }

    snprintf(payload, sizeof(payload), "%u\t%s\t%s", type, description, title);
    veVariantHeapStr(&v, payload);

    msg = dbus_message_new_method_call("com.victronenergy.platform",
                                       "/Notifications/Inject",
                                       "com.victronenergy.BusItem", "SetValue");
    if (!msg) {
        return;
    }

    dbus_message_iter_init_append(msg, &itt);
    veDbusMsgAppendVeVariant(&itt, &v);
    veDbusSend(dbus->service.conn, msg);

    dbus_message_unref(msg);
    veVariantFree(&v);
}