#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "servicemanager.h"
}

class ServiceManagerTest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();

        canOpenInit();
        serviceManagerInit();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }
};

TEST_F(ServiceManagerTest, dbusConnectFailure) {
    veDbusConnectString_fake.return_val = NULL;

    ASSERT_EXIT(serviceManagerInit();, ::testing::ExitedWithCode(1), "");
}

TEST_F(ServiceManagerTest, dbusChangeNameFailure) {
    veDbusChangeName_fake.return_val = veFalse;

    ASSERT_EXIT(serviceManagerInit();, ::testing::ExitedWithCode(3), "");
}

TEST_F(ServiceManagerTest, scan) {
    VeVariant v;

    serviceManagerInit();

    EXPECT_EQ(serviceManager.scan->variant.value.UN8, 0);
    EXPECT_EQ(serviceManager.scanProgress->variant.value.UN8, 0);
    EXPECT_EQ(serviceManager.discoveredNodeIds.count, 0);

    veItemSet(serviceManager.scan, veVariantUn8(&v, 1));
    EXPECT_EQ(serviceManager.scan->variant.value.UN8, 1);

    for (unsigned int i = 1; i <= 127; i++) {
        canOpenTx();

        if (i == 0x1 || i == 0x26) {
            this->canMsgReadQueue.push_back(
                {.canId = 0x580 + i,
                 .length = 8,
                 .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
            canOpenRx();
        } else {
            pltGetCount1ms_fake.return_val = i * 50;
            canOpenTx();
        }

        if (i < 127) {
            EXPECT_EQ(serviceManager.scanProgress->variant.value.UN8,
                      i * 100 / 127);
        }
    }

    EXPECT_EQ(serviceManager.scan->variant.value.UN8, 0);
    EXPECT_EQ(serviceManager.scanProgress->variant.value.UN8, 0);
    EXPECT_EQ(serviceManager.discoveredNodeIds.count, 2);
    EXPECT_EQ(serviceManager.discoveredNodeIds.data[0], 0x1);
    EXPECT_EQ(serviceManager.discoveredNodeIds.data[1], 0x26);
    EXPECT_STREQ(
        (const char *)serviceManager.discoveredNodes->variant.value.Ptr,
        "1,38");
}