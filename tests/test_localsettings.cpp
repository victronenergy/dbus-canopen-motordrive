#include "BaseFixture.hpp"

extern "C" {
#include "localsettings.h"
}

class LocalSettingsTest : public BaseFixture {};

TEST_F(LocalSettingsTest, init) {
    char debug[1024];
    VeItem *item;

    localSettingsInit();

    veItemUid(localSettings, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/com.victonenergy.consumer");
}

TEST_F(LocalSettingsTest, failureToConnectToDbus) {
    veDbusGetDefaultBus_fake.return_val = NULL;

    ASSERT_EXIT(localSettingsInit();, ::testing::ExitedWithCode(1), "");
}

TEST_F(LocalSettingsTest, failureToAddRemoteService) {
    veDbusAddRemoteService_fake.return_val = NULL;

    ASSERT_EXIT(localSettingsInit();, ::testing::ExitedWithCode(4), "");
}