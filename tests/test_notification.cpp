#include "BaseFixture.hpp"

extern "C" {
#include "notification.h"
}

class NotificationTest : public BaseFixture {
  protected:
    void SetUp() override {
        BaseFixture::SetUp();

        notificationsInit();
    }
};

TEST_F(NotificationTest, queueNotificationMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(
        queueNotification(1, NOTIFICATION_TYPE_ERROR, "title", "description"),
        ::testing::ExitedWithCode(5), "");
}

TEST_F(NotificationTest, queueNotificationFirstStrdupFailure) {
    _strdup_fake.custom_fake = NULL;
    _strdup_fake.return_val = NULL;

    ASSERT_EXIT(
        queueNotification(1, NOTIFICATION_TYPE_ERROR, "title", "description"),
        ::testing::ExitedWithCode(5), "");
}

static char *strdup_second_failure(const char *s) {
    if (_strdup_fake.call_count == 2) {
        return NULL;
    }
    return strdup(s);
};

TEST_F(NotificationTest, queueNotificationSecondStrdupFailure) {

    _strdup_fake.custom_fake = strdup_second_failure;

    ASSERT_EXIT(
        queueNotification(1, NOTIFICATION_TYPE_ERROR, "title", "description"),
        ::testing::ExitedWithCode(5), "");
}