#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "drivers/curtis_e.h"
#include "node.h"
#include "servicemanager.h"
}

class CurtisETest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();

        canOpenInit();
        nodesInit();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }
};

TEST_F(CurtisETest, createDriverContextMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(curtisEDriver.createDriverContext(&nodes[0]);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(CurtisETest, readSuccess) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 6);

    std::vector<VeRawCanMsg> queue;
    // Swap Motor Direction
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x4C, 0x32, 0x00, 0x20, 0x0D, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x9E, 0x35, 0x00, 0x64, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x0B, 0x32, 0x00, 0xFA, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x2A, 0x32, 0x00, 0x2C, 0x01, 0x00, 0x00}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 6);

    message = this->canMsgSentLog.at(0);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x6C);
    EXPECT_EQ(message.mdata[2], 0x30);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(1);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x4C);
    EXPECT_EQ(message.mdata[2], 0x32);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(2);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x9E);
    EXPECT_EQ(message.mdata[2], 0x35);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(3);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x07);
    EXPECT_EQ(message.mdata[2], 0x32);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(4);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x0B);
    EXPECT_EQ(message.mdata[2], 0x32);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(5);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x2A);
    EXPECT_EQ(message.mdata[2], 0x32);
    EXPECT_EQ(message.mdata[3], 0x00);

    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_EQ(nodes[0].device->current->variant.value.Float, 10.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, 525);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.Float, 25.0F);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.Float,
              30.0F);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

TEST_F(CurtisETest, skipResponseOnDisconnect) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 6);

    std::vector<VeRawCanMsg> queue;
    // Swap Motor Direction
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x4C, 0x32, 0x00, 0x20, 0x0D, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x9E, 0x35, 0x00, 0x64, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x0B, 0x32, 0x00, 0xFA, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x2A, 0x32, 0x00, 0x2C, 0x01, 0x00, 0x00}});
    disconnectFromNode(1);

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 6);

    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
}

TEST_F(CurtisETest, readErrorTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 6);

    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 6);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(CurtisETest, readSwapMotorDirectionOnce) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    ((CurtisEContext *)nodes[0].device->driverContext)->swapMotorDirection = 1;
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 5);
}

TEST_F(CurtisETest, motorDirection) {
    VeVariant v;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    // 500 rpm, direction not inverted, motor not swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);

    // -500 rpm, direction not inverted, motor not swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0x0C, 0xFE, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);

    // 0 rpm, direction not inverted, motor not swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 0);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 0);

    // 500 rpm, direction inverted, motor not swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veTrue));
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);

    // -500 rpm, direction not inverted, motor not swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veTrue));
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0x0C, 0xFE, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);

    // 500 rpm, direction not inverted, motor swapped
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    ((CurtisEContext *)nodes[0].device->driverContext)->swapMotorDirection = 1;
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veFalse));
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);
}

TEST_F(CurtisETest, currentDeadband) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 6);

    std::vector<VeRawCanMsg> queue;
    // Swap Motor Direction
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x4C, 0x32, 0x00, 0x20, 0x0D, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x9E, 0x35, 0x00, 0x01, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x0B, 0x32, 0x00, 0xFA, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x2A, 0x32, 0x00, 0x2C, 0x01, 0x00, 0x00}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 6);
    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_EQ(nodes[0].device->current->variant.value.Float, 0.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, 0);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.Float, 25.0F);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.Float,
              30.0F);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

TEST_F(CurtisETest, currentNegative) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
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
         .mdata = {0x42, 0x64, 0x34, 0x00, 0x00, 0xFD, 0xBB, 0x00}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 6);

    std::vector<VeRawCanMsg> queue;
    // Swap Motor Direction
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x4C, 0x32, 0x00, 0x20, 0x0D, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x9E, 0x35, 0x00, 0x9C, 0xFF, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x07, 0x32, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x0B, 0x32, 0x00, 0xFA, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x2A, 0x32, 0x00, 0x2C, 0x01, 0x00, 0x00}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 6);
    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_EQ(nodes[0].device->current->variant.value.Float, -10.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, -525);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.Float, 25.0F);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.Float,
              30.0F);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}