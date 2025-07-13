#include <localsettings.h>
#include <logger.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_values.h>

VeItem *localSettings;

void localSettingsInit(void) {
    struct VeDbus *dbus;

    dbus = veDbusGetDefaultBus();
    veDbusSetListeningDbus(dbus);

    localSettings =
        veItemGetOrCreateUid(veValueTree(), "com.victonenergy.consumer");
    if (!veDbusAddRemoteService("com.victronenergy.settings", localSettings,
                                veTrue)) {
        error("veDbusAddRemoteService failed");
        pltExit(4);
    }
}