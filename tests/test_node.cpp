#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "node.h"
#include "servicemanager.h"
}

class NodeTest : public CanFixture {
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

TEST_F(NodeTest, connectToNodeMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(connectToNode(1);, ::testing::ExitedWithCode(5), "");
}

TEST_F(NodeTest, connectToNodeTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    connectToNode(1);

    canOpenTx();
    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, connectToNodeSerialNumberTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    connectToNode(1);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x47, 0x65, 0x6E, 0x34}});
    canOpenRx();
    canOpenTx();
    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, connectToNodeCustomSerialNumberTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    connectToNode(1);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    canOpenRx();
    canOpenTx();
    pltGetCount1ms_fake.return_val = 50;
    canOpenTx();

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, connectToNodeDeviceMallocFailure) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    connectToNode(1);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x45, 0x4D, 0x44, 0x49}});
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenRx();, ::testing::ExitedWithCode(5), "");
}

TEST_F(NodeTest, connectToNodeCustomSerialNumberMallocFailure) {
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
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(canOpenRx();, ::testing::ExitedWithCode(5), "");
}

TEST_F(NodeTest, connectToNodeSuccessSevcon) {
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

    disconnectFromNode(1);

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, connectToNodeSuccessCurtisF) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    connectToNode(1);

    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x08, 0x10, 0x00, 0x41, 0x43, 0x20, 0x46}});
    canOpenRx();
    canOpenTx();

    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();

    EXPECT_EQ(nodes[0].connected, veTrue);

    disconnectFromNode(1);

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, disconnectFromNodeNotConnected) {
    EXPECT_EQ(nodes[0].connected, veFalse);

    disconnectFromNode(1);

    EXPECT_EQ(nodes[0].connected, veFalse);
}

TEST_F(NodeTest, connectToDiscoveredNodes) {
    VeRawCanMsg message;

    serviceManagerInit();

    un8ArrayAdd(&serviceManager.discoveredNodeIds, 0x01);
    un8ArrayAdd(&serviceManager.discoveredNodeIds, 0x26);

    nodes[0].connected = veTrue;

    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    connectToDiscoveredNodes();
    canOpenTx();

    EXPECT_EQ(this->canMsgSentLog.size(), 1);

    message = this->canMsgSentLog.back();
    EXPECT_EQ(message.canId, 0x626);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x08);
    EXPECT_EQ(message.mdata[2], 0x10);
    EXPECT_EQ(message.mdata[3], 0x00);
    EXPECT_EQ(message.mdata[4], 0x00);
    EXPECT_EQ(message.mdata[5], 0x00);
    EXPECT_EQ(message.mdata[6], 0x00);
    EXPECT_EQ(message.mdata[7], 0x00);
}

TEST_F(NodeTest, readFromConnectedNodes) {
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

    veBool fast = veFalse;
    readFromConnectedNodes(fast);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x01, 0x20, 0x00, 0x00, 0x00}});
    // Battery Current
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x02, 0x10, 0x00, 0x00, 0x00}});
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0x40, 0x00, 0x00, 0x00}});
    // Motor Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x46, 0x03, 0x1B, 0x00, 0x00, 0x00}});
    // Motor Torque
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x02, 0x46, 0x0C, 0x30, 0x00, 0x00, 0x00}});
    // Controller Temperature
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x00, 0x51, 0x04, 0x1D, 0x00, 0x00, 0x00}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }
    canOpenTx();

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
}

TEST_F(NodeTest, readFromConnectedNodesFast) {
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

    veBool fast = veTrue;
    readFromConnectedNodes(fast);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 2);

    std::vector<VeRawCanMsg> queue;
    // Motor RPM
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x42, 0x6c, 0x60, 0x00, 0x40, 0x00, 0x00, 0x00}});

    while (!queue.empty()) {
        this->canMsgReadQueue.push_back(queue.front());
        queue.erase(queue.begin());
        canOpenTx();
        canOpenRx();
    }
    canOpenTx();

    EXPECT_EQ(this->canMsgSentLog.size(), 1);

    message = this->canMsgSentLog.at(0);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x6c);
    EXPECT_EQ(message.mdata[2], 0x60);
    EXPECT_EQ(message.mdata[3], 0x00);

    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
}

TEST_F(NodeTest, readFromConnectedNodesBusy) {
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

    veBool fast = veFalse;
    readFromConnectedNodes(fast);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    readFromConnectedNodes(fast);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);
}

TEST_F(NodeTest, readFromConnectedNodesTimeout) {
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

    veBool fast = veFalse;
    readFromConnectedNodes(fast);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 7);

    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(nodes[0].connected, veFalse);
}