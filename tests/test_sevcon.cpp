#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "drivers/sevcon.h"
#include "node.h"
#include "servicemanager.h"
}

class SevconTest : public CanFixture {
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

TEST_F(SevconTest, readSuccess) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
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
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x01, 0x48, 0x03, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x02, 0xA0, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x46, 0x03, 0x19, 0x00, 0x00, 0x00}});
    // Motor Torque
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x02, 0x46, 0x0C, 0x50, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x04, 0x1E, 0x00, 0x00, 0x00}});

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
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x51);
    EXPECT_EQ(message.mdata[3], 0x01);

    message = this->canMsgSentLog.at(1);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x51);
    EXPECT_EQ(message.mdata[3], 0x02);

    message = this->canMsgSentLog.at(2);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x6c);
    EXPECT_EQ(message.mdata[2], 0x60);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(3);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x46);
    EXPECT_EQ(message.mdata[3], 0x03);

    message = this->canMsgSentLog.at(4);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x02);
    EXPECT_EQ(message.mdata[2], 0x46);
    EXPECT_EQ(message.mdata[3], 0x0C);

    message = this->canMsgSentLog.at(5);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x00);
    EXPECT_EQ(message.mdata[2], 0x51);
    EXPECT_EQ(message.mdata[3], 0x04);

    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_EQ(nodes[0].device->current->variant.value.Float, 10.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, 525);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.SN16, 25);
    EXPECT_EQ(nodes[0].device->motorTorque->variant.value.UN16, 80);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.SN16, 30);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

TEST_F(SevconTest, skipResponseOnDisconnect) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
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
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x01, 0x48, 0x03, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x02, 0xA0, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x46, 0x03, 0x19, 0x00, 0x00, 0x00}});
    // Motor Torque
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x02, 0x46, 0x0C, 0x50, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x04, 0x1E, 0x00, 0x00, 0x00}});

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

TEST_F(SevconTest, motorDirection) {
    VeVariant v;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    // 500 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);

    // -500 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0x0C, 0xFE, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);

    // 0 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 0);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 0);

    // 500 rpm, direction inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veTrue));
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);

    // -500 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veTrue));
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0x0C, 0xFE, 0x00, 0x00}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}