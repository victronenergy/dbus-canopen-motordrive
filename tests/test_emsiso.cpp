#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "drivers/emsiso.h"
#include "node.h"
#include "servicemanager.h"
}

FAKE_VALUE_FUNC3(veBool, testEmsisoSetter, struct VeItem *, void *,
                 VeVariant *);
static char payload[1024];
static veBool testEmsisoSetterLocal(struct VeItem *item, void *context,
                                    VeVariant *v) {
    strcpy(payload, (const char *)v->value.CPtr);
    return veTrue;
}

class EmsisoTest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();
        RESET_FAKE(testEmsisoSetter);

        testEmsisoSetter_fake.custom_fake = testEmsisoSetterLocal;

        canOpenInit();
        nodesInit();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }
};

TEST_F(EmsisoTest, readSuccess) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x07, 0x20, 0x01, 0x41, 0x42, 0x43, 0x44}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xf0, 0x31, 0x08, 0x00, 0x00, 0x52, 0x42}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xf0, 0x31, 0x07, 0x00, 0x00, 0x20, 0x41}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0x43}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x0b, 0x00, 0x00, 0xc8, 0x41}});
    // Motor Torque
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x03, 0x00, 0x00, 0xa0, 0x42}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xcd, 0x31, 0x00, 0x00, 0x00, 0xf0, 0x41}});
    // Coolant Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xcf, 0x31, 0x00, 0x00, 0x00, 0xd4, 0x41}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 7);

    message = this->canMsgSentLog.at(0);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0xf0);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x08);

    message = this->canMsgSentLog.at(1);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0xf0);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x07);

    message = this->canMsgSentLog.at(2);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x01);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x01);

    message = this->canMsgSentLog.at(3);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x01);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x0b);

    message = this->canMsgSentLog.at(4);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x01);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x03);

    message = this->canMsgSentLog.at(5);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0xcd);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(6);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0xcf);
    EXPECT_EQ(message.mdata[2], 0x31);
    EXPECT_EQ(message.mdata[3], 0x00);

    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_EQ(nodes[0].device->current->variant.value.Float, 10.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, 525);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.Float, 25.0F);
    EXPECT_EQ(nodes[0].device->motorTorque->variant.value.Float, 80.0F);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.Float,
              30.0F);
    EXPECT_EQ(nodes[0].device->coolantTemperature->variant.value.Float, 26.5F);
}

TEST_F(EmsisoTest, skipResponseOnDisconnect) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x07, 0x20, 0x01, 0x41, 0x42, 0x43, 0x44}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xf0, 0x31, 0x08, 0x00, 0x00, 0x52, 0x42}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xf0, 0x31, 0x07, 0x00, 0x00, 0x20, 0x41}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0x43}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x0b, 0x00, 0x00, 0xc8, 0x41}});
    // Motor Torque
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x03, 0x00, 0x00, 0xa0, 0x42}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xcd, 0x31, 0x00, 0x00, 0x00, 0xf0, 0x41}});
    // Coolant Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0xcf, 0x31, 0x00, 0x00, 0x00, 0xd4, 0x41}});
    disconnectFromNode(1);

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 7);

    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
}

TEST_F(EmsisoTest, readErrorTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x07, 0x20, 0x01, 0x41, 0x42, 0x43, 0x44}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 7);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(EmsisoTest, motorDirection) {
    VeVariant v;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x07, 0x20, 0x01, 0x41, 0x42, 0x43, 0x44}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    // 500 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0x43}});
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
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0xc3}});
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
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0x00, 0x00}});
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
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0x43}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 1);

    // -500 rpm, direction inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    veItemOwnerSet(nodes[0].device->motorDirectionInverted,
                   veVariantSn32(&v, veTrue));
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x01, 0x31, 0x01, 0x00, 0x00, 0xfa, 0xc3}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

TEST_F(EmsisoTest, emcyMessage) {
    VeRawCanMsg message;
    VeItem *item;
    VeVariant v;

    item = veItemGetOrCreateUid(
        veValueTree(), "com.victronenergy.platform/Notifications/Inject");
    EXPECT_NE(item, nullptr);
    canOpenRegisterEmcyHandler(nodesEmcyHandler, NULL);
    veItemSetSetter(item, testEmsisoSetter, NULL);
    EXPECT_EQ(testEmsisoSetter_fake.call_count, 0);

    // error
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    // Ignore error node isn't connected
    EXPECT_EQ(testEmsisoSetter_fake.call_count, 0);

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToNode(1);
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x07, 0x20, 0x01, 0x41, 0x42, 0x43, 0x44}});
    canOpenRx();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    // Generic Error
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    // Ignored; no handler
    EXPECT_EQ(testEmsisoSetter_fake.call_count, 0);
}