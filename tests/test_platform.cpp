#include "BaseFixture.hpp"

extern "C" {
#include "platform.h"
#include <velib/types/ve_dbus_item.h>
#include <velib/types/ve_item_def.h>
#include <velib/types/ve_values.h>
}

FAKE_VALUE_FUNC3(veBool, testSetter, struct VeItem *, void *, VeVariant *);
static char payload[1024];
static veBool testSetterLocal(struct VeItem *item, void *context,
                              VeVariant *v) {
    strcpy(payload, (const char *)v->value.CPtr);
    return veTrue;
}

class PlatformTest : public BaseFixture {
  protected:
    void SetUp() override {
        BaseFixture::SetUp();
        RESET_FAKE(testSetter);

        testSetter_fake.custom_fake = testSetterLocal;
    }
};

TEST_F(PlatformTest, init) {
    VeItem *item;

    item = veItemByUid(veValueTree(), "com.victronenergy.platform");
    EXPECT_EQ(item, nullptr);

    platformInit();

    item = veItemByUid(veValueTree(), "com.victronenergy.platform");
    EXPECT_NE(item, nullptr);

    item = veItemByUid(item, "Notifications/Inject");
    EXPECT_NE(item, nullptr);
}

TEST_F(PlatformTest, failureToAddRemoteService) {
    veDbusAddRemoteService_fake.return_val = NULL;

    platformInit();

    // Warning only, doesn't exit
}

TEST_F(PlatformTest, injectPlatformNotificationSuccess) {
    VeItem *item;

    item = veItemGetOrCreateUid(
        veValueTree(), "com.victronenergy.platform/Notifications/Inject");
    EXPECT_NE(item, nullptr);

    veItemSetSetter(item, testSetter, NULL);

    injectPlatformNotification(NOTIFICATION_TYPE_INFO, "Test Title",
                               "Test Description");

    EXPECT_EQ(testSetter_fake.call_count, 1);

    EXPECT_STREQ(payload, "2\tTest Description\tTest Title");
}