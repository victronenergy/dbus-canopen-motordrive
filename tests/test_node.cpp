#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "node.h"
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

TEST_F(NodeTest, connectToNodeDeviceMallocFailure) {
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