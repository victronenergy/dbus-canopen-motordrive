#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
}

// Segmented read
// vecan0       60A   [8]  40 08 10 00 00 00 00 00
// vecan0       58A   [8]  41 08 10 00 0E 00 00 00
// vecan0       60A   [8]  60 00 00 00 00 00 00 00
// vecan0       58A   [8]  00 47 65 6E 34 20 53 69
// vecan0       60A   [8]  70 00 00 00 00 00 00 00
// vecan0       58A   [8]  11 6D 75 6C 61 74 6F 72

// Expeditive segmented read
// vecan0       60A   [8]  40 08 10 00 00 00 00 00
// vecan0       58A   [8]  43 08 10 00 47 65 6E 34

FAKE_VOID_FUNC1(testCallback, CanOpenPendingSdoRequest *);
FAKE_VOID_FUNC2(testErrorCallback, CanOpenPendingSdoRequest *, CanOpenError);
FAKE_VOID_FUNC3(testEMCYCallback, void *, un8, VeRawCanMsg *);
static SdoMessage sdoMessage;
static void testCallbackLocal(CanOpenPendingSdoRequest *request) {
    memcpy(&sdoMessage, &request->response, sizeof(SdoMessage));
}

class CanopenTest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();
        RESET_FAKE(testCallback);
        RESET_FAKE(testErrorCallback);

        testCallback_fake.custom_fake = testCallbackLocal;

        canOpenInit();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }
};

TEST_F(CanopenTest, queueCallbackAsync) {
    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenQueueCallbackAsync(NULL, testCallback);
    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    // Unrelated messages should not trigger the callback
    this->canMsgReadQueue.push_back(
        {.canId = 0x123,
         .length = 8,
         .mdata = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});
    this->canMsgReadQueue.push_back(
        {.canId = 0x580,
         .length = 8,
         .mdata = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(testCallback_fake.call_count, 0);

    canOpenTx();
    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);
}

TEST_F(CanopenTest, queueCallbackAsyncMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenQueueCallbackAsync(NULL, testCallback);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(CanopenTest, noRequestInQueue) {
    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    this->canMsgReadQueue.push_back(
        {.canId = 0x123,
         .length = 8,
         .mdata = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    canOpenTx();
}

TEST_F(CanopenTest, veCanSendRetries) {
    veCanSend_fake.custom_fake = NULL;
    veCanSend_fake.return_val = veFalse;

    canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback, testErrorCallback);

    canOpenTx();
}

TEST_F(CanopenTest, readSdoAsync) {
    VeRawCanMsg message;

    canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback, testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x04);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x34, 0x12, 0x00, 0x00}});

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    EXPECT_EQ(sdoMessage.data, 0x1234);
}

TEST_F(CanopenTest, readSdoAsyncSendAbortOnErrorResponse) {
    VeRawCanMsg message;

    canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback, testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x04);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x18, 0x10, 0x04, 0x34, 0x12, 0x00, 0x00}});

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x80);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x04);
    EXPECT_EQ(message.mdata[4], 0x05);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x04);
    EXPECT_EQ(message.mdata[7], 0x05);
}

TEST_F(CanopenTest, readSdoAsyncErrorCallback) {
    VeRawCanMsg message;

    canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback, testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x18);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x04);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x18, 0x10, 0x04, 0x34, 0x12, 0x00, 0x00}});

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 1);
}

TEST_F(CanopenTest, readSdoAsyncTimeout) {
    canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback, testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();
    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    pltGetCount1ms_fake.return_val = 49;
    canOpenTx();
    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 1);
}

TEST_F(CanopenTest, readSdoAsyncMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenReadSdoAsync(1, 0x1018, 0x04, NULL, testCallback,
                                    testErrorCallback);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(CanopenTest, writeSdoAsync) {
    VeRawCanMsg message;

    canOpenWriteSdoAsync(1, 0x5300, 0x02, 0x00, NULL, testCallback,
                         testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x23);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x53);
    EXPECT_EQ(message.mdata[3], 0x02);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x60, 0x00, 0x53, 0x02, 0x00, 0x00, 0x00, 0x00}});

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);
}

TEST_F(CanopenTest, writeSdoAsyncErrorCallback) {
    VeRawCanMsg message;

    canOpenWriteSdoAsync(1, 0x5300, 0x02, 0x00, NULL, testCallback,
                         testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x23);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x53);
    EXPECT_EQ(message.mdata[3], 0x02);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x00, 0x53, 0x02, 0x34, 0x12, 0x00, 0x00}});

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 1);
}

TEST_F(CanopenTest, writeSdoAsyncMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenWriteSdoAsync(1, 0x5300, 0x02, 0x00, NULL, testCallback,
                                     testErrorCallback);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(CanopenTest, readSegmentedSdoAsync) {
    VeRawCanMsg message;
    un8 buffer[256];
    un8 length;

    canOpenReadSegmentedSdoAsync(1, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x0E, 0x00, 0x00, 0x00}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x60);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x00);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x47, 0x65, 0x6E, 0x34, 0x20, 0x53, 0x69}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x70);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x00);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x6D, 0x75, 0x6C, 0x61, 0x74, 0x6F, 0x72}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x60);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x00);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x1B, 0x34, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    buffer[length] = 0;
    EXPECT_EQ(length, 16);
    EXPECT_STREQ((char *)buffer, "Gen4 Simulator44");
}

TEST_F(CanopenTest, readSegmentedSdoAsyncTwoBackToBack) {
    un8 buffer[256];
    un8 length;

    canOpenReadSegmentedSdoAsync(1, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);
    canOpenReadSegmentedSdoAsync(2, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    buffer[length] = 0;
    EXPECT_EQ(length, 4);
    EXPECT_STREQ((char *)buffer, "Gen4");

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x582,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x0E, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    this->canMsgReadQueue.push_back(
        {.canId = 0x582,
         .length = 8,
         .mdata = {0x00, 0x47, 0x65, 0x6E, 0x34, 0x20, 0x53, 0x69}});
    canOpenRx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x582,
         .length = 8,
         .mdata = {0x1B, 0x34, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 2);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    buffer[length] = 0;
    EXPECT_EQ(length, 9);
    EXPECT_STREQ((char *)buffer, "Gen4 Si44");
}

TEST_F(CanopenTest, readSegmentedSdoAsyncExpedited) {
    VeRawCanMsg message;
    un8 buffer[256];
    un8 length;

    canOpenReadSegmentedSdoAsync(1, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 1);
    EXPECT_EQ(testErrorCallback_fake.call_count, 0);

    buffer[length] = 0;
    EXPECT_EQ(length, 4);
    EXPECT_STREQ((char *)buffer, "Gen4");
}

TEST_F(CanopenTest, readSegmentedSdoAsyncErrorResponse) {
    VeRawCanMsg message;
    un8 buffer[256];
    un8 length;

    canOpenReadSegmentedSdoAsync(1, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x80, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 1);
}

TEST_F(CanopenTest, readSegmentedSdoAsyncMaxLength) {
    VeRawCanMsg message;
    un8 buffer[10];
    un8 length;

    canOpenReadSegmentedSdoAsync(1, 0x1008, 0x00, NULL, buffer, &length,
                                 sizeof(buffer) - 1, testCallback,
                                 testErrorCallback);

    EXPECT_NE(canOpenState.pendingSdoRequests->first, nullptr);

    canOpenTx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x41, 0x08, 0x10, 0x00, 0x0E, 0x00, 0x00, 0x00}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x60);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x00);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x47, 0x65, 0x6E, 0x34, 0x20, 0x53, 0x69}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x70);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x00);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x00, 0x6D, 0x75, 0x6C, 0x61, 0x74, 0x6F, 0x72}});
    canOpenRx();

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x80);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x05);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x04);
    EXPECT_EQ(message.mdata[7], 0x05);

    EXPECT_EQ(testCallback_fake.call_count, 0);
    EXPECT_EQ(testErrorCallback_fake.call_count, 1);

    buffer[length] = 0;
    EXPECT_EQ(length, 9);
    EXPECT_STREQ((char *)buffer, "Gen4 Simu");
}

TEST_F(CanopenTest, readSegmentedSdoAsyncMallocFailure) {
    un8 buffer[10];
    un8 length;

    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenReadSegmentedSdoAsync(1, 0x1018, 0x04, NULL, buffer,
                                             &length, sizeof(buffer) - 1,
                                             testCallback, testErrorCallback);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(CanopenTest, emcyHandler) {
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    canOpenRegisterEmcyHandler(testEMCYCallback, NULL);

    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(testEMCYCallback_fake.call_count, 1);
    EXPECT_EQ(testEMCYCallback_fake.arg0_val, nullptr);
    EXPECT_EQ(testEMCYCallback_fake.arg1_val, 1);
}