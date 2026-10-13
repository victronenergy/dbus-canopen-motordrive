#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "discovery.h"
#include <drivers/curtis_e.h>
#include <drivers/curtis_f.h>
#include <drivers/dmc.h>
#include <drivers/sevcon.h>
}

FAKE_VOID_FUNC3(testDiscoverySuccessCallback, un8, void *, Driver *);
FAKE_VOID_FUNC2(testDiscoveryErrorCallback, un8, void *);

class DiscoveryTest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();
        RESET_FAKE(testDiscoverySuccessCallback);
        RESET_FAKE(testDiscoveryErrorCallback);

        canOpenInit();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }
};

TEST_F(DiscoveryTest, discoverNodeMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(discoverNode(1, testDiscoverySuccessCallback,
                             testDiscoveryErrorCallback, NULL);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(DiscoveryTest, discoverNodeErrorProductNameTimeout) {
    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorVendorIdTimeout) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x01);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorVendorIdNotCurtis) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorCurtisModelNumberTimeout) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x64);
    EXPECT_EQ(message.mdata[2], 0x34);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorCurtisModelNumberInvalidLow) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorCurtisModelNumberInvalidHigh) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0x00, 0xFF, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeSuccessCurtisE1232) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    // Curtis 1232: 0xBBFD00
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &curtisEDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeSuccessCurtisE1234) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    // Curtis 1234: 0xBC4B20
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x20, 0x4B, 0xBC, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &curtisEDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeSuccessCurtisE1236) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    // Curtis 1236: 0xBC9940
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x40, 0x99, 0xBC, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &curtisEDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeSuccessCurtisE1238) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x01, 0x49, 0x43, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    // Curtis 1238: 0xBCE760
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x60, 0xE7, 0xBC, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &curtisEDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeErrorUnknownName) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x33}});
    // Gen3
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorUnknownNameShort) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x47, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x00}});
    // Gen
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeSuccessSevcon) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &sevconDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeSuccessSevconWithBorgWarnerControllerName) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x28, 0x00, 0x00, 0x00}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x42, 0x6F, 0x72, 0x67, 0x57, 0x61, 0x72}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x10, 0x6E, 0x65, 0x72, 0x20, 0x47, 0x65, 0x6E}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x34, 0x20, 0x28, 0x50, 0x4D, 0x41, 0x43}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x10, 0x29, 0x54, 0x75, 0x65, 0x20, 0x4F, 0x63}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x74, 0x20, 0x31, 0x35, 0x20, 0x31, 0x35}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x15, 0x3A, 0x34, 0x31, 0x3A, 0x00, 0x01, 0x35}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &sevconDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

// All four frames below were captured from a DMC Sigma2N pair, firmware
// V03.03.01, on a 6-phase bench setup. The controller announces a 20 byte
// segmented upload of 0x1008 and then aborts the segment request, so
// discovery has to identify it from the identity object instead.
TEST_F(DiscoveryTest, discoverNodeSuccessDmcSigma2NViaVendorId) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x01);

    // Vendor ID 0x04F1
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x18, 0x10, 0x01, 0xF1, 0x04, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x02);

    // Product code 0x22488014
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x18, 0x10, 0x02, 0x14, 0x80, 0x48, 0x22}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &dmcDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}

TEST_F(DiscoveryTest, discoverNodeErrorDmcProductCodeUnknown) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x18, 0x10, 0x01, 0xF1, 0x04, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x18, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeErrorDmcProductCodeTimeout) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x14, 0x00, 0x00, 0x00}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x06}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x18, 0x10, 0x01, 0xF1, 0x04, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();

    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 1);
}

TEST_F(DiscoveryTest, discoverNodeSuccessCurtisF) {
    VeRawCanMsg message;

    discoverNode(1, testDiscoverySuccessCallback, testDiscoveryErrorCallback,
                 NULL);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 0);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x41, 0x43, 0x20, 0x46}});
    canOpenRx();

    EXPECT_EQ(testDiscoverySuccessCallback_fake.call_count, 1);
    EXPECT_EQ(testDiscoverySuccessCallback_fake.arg2_val, &curtisFDriver);
    EXPECT_EQ(testDiscoveryErrorCallback_fake.call_count, 0);
}