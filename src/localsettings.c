#include <localsettings.h>
#include <logger.h>
#include <velib/platform/plt.h>
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_values.h>

VeItem *localSettings;

void localSettingsInit(void) {
    localSettings =
        veItemGetOrCreateUid(veValueTree(), "com.victonenergy.settings");
    if (!veDbusAddRemoteService("com.victronenergy.settings", localSettings,
                                veTrue)) {
        error("veDbusAddRemoteService failed for com.victronenergy.settings");
        pltExit(4);
    }
}