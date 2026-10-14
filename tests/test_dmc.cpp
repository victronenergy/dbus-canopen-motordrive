#include "CanFixture.hpp"

extern "C" {
#include "canopen.h"
#include "drivers/dmc.h"
#include "node.h"
#include "servicemanager.h"
}

static char lastTitle[1024];
static char lastDescription[1024];
static void injectPlatformNotificationLocal(NotificationType type,
                                            char const *title,
                                            char const *description) {
    strncpy(lastTitle, title, sizeof(lastTitle));
    strncpy(lastDescription, description, sizeof(lastDescription));
}

class DmcTest : public CanFixture {
  protected:
    void SetUp() override {
        CanFixture::SetUp();

        injectPlatformNotification_fake.custom_fake =
            injectPlatformNotificationLocal;

        canOpenInit();
        nodesInit();
        notificationsInit();
        dmcResetDualMotorConfigurations();
    }

    void TearDown() override {
        CanFixture::TearDown();

        listDestroy(canOpenState.pendingSdoRequests);
        canOpenState.pendingSdoRequests = NULL;
    }

    // The controller announces a 20 byte segmented upload of 0x1008 and then
    // aborts the segment request, so discovery identifies it from the identity
    // object. Frames captured from a Sigma2N pair, firmware V03.03.01.
    void connectToDmcNode(un8 dualMotorConfiguration = 0) {
        connectToNode(1);
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
        // Vendor ID 0x04F1
        this->canMsgReadQueue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x43, 0x18, 0x10, 0x01, 0xF1, 0x04, 0x00, 0x00}});
        canOpenRx();
        canOpenTx();
        // Product code 0x22488014
        this->canMsgReadQueue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x43, 0x18, 0x10, 0x02, 0x14, 0x80, 0x48, 0x22}});
        canOpenRx();
        canOpenTx();
        // Dual motor configuration, 0 for a single motor
        this->canMsgReadQueue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4F, 0x03, 0x24, 0x00, dualMotorConfiguration, 0x00,
                       0x00, 0x00}});
        canOpenRx();
        canOpenTx();
        this->canMsgReadQueue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x43, 0x18, 0x10, 0x04, 0x01, 0x00, 0x00, 0x00}});
        canOpenRx();
    }

    // One full read cycle with the six measurement objects at fixed values
    // and a chosen fault code and subcode.
    void readCycleWithFault(un8 faultCode, un16 faultSubcode) {
        std::vector<VeRawCanMsg> queue;
        nodes[0].device->driver->readRoutine(&nodes[0]);
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4B, 0x3F, 0x38, 0x00, 0x0D, 0x02, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4B, 0x3E, 0x38, 0x00, 0x64, 0x00, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4F, 0x36, 0x38, 0x00, 0x4C, 0x00, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4B, 0x1C, 0x41, 0x00, 0x20, 0x03, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4F, 0x37, 0x38, 0x00, 0x51, 0x00, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4F, 0x40, 0x38, 0x00, faultCode, 0x00, 0x00, 0x00}});
        queue.push_back(
            {.canId = 0x581,
             .length = 8,
             .mdata = {0x4B, 0x41, 0x38, 0x00, (un8)(faultSubcode & 0xFF),
                       (un8)(faultSubcode >> 8), 0x00, 0x00}});
        drainQueue(queue);
        pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
        processPendingNotifications();
    }

    void drainQueue(std::vector<VeRawCanMsg> &queue) {
        while (!queue.empty()) {
            this->canMsgReadQueue.push_back(queue.front());
            queue.erase(queue.begin());
            canOpenTx();
            canOpenRx();
        }
    }
};

TEST_F(DmcTest, createDriverContextMallocFailure) {
    _malloc_fake.custom_fake = NULL;
    _malloc_fake.return_val = NULL;

    ASSERT_EXIT(dmcDriver.createDriverContext(&nodes[0]);
                , ::testing::ExitedWithCode(5), "");
}

TEST_F(DmcTest, readSuccess) {
    VeRawCanMsg message;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage, 525 = 52.5V
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3F, 0x38, 0x00, 0x0D, 0x02, 0x00, 0x00}});
    // Battery Current, 100 = 10.0A
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3E, 0x38, 0x00, 0x64, 0x00, 0x00, 0x00}});
    // Motor RPM, 500
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature, 76 = 25°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x36, 0x38, 0x00, 0x4C, 0x00, 0x00, 0x00}});
    // Motor Torque, 800 = 80.0Nm
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x1C, 0x41, 0x00, 0x20, 0x03, 0x00, 0x00}});
    // Controller Temperature, 81 = 30°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x37, 0x38, 0x00, 0x51, 0x00, 0x00, 0x00}});

    // Fault code 0, no fault
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x40, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Fault subcode 0
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x41, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});

    drainQueue(queue);

    EXPECT_EQ(this->canMsgSentLog.size(), 8);

    message = this->canMsgSentLog.at(0);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x3F);
    EXPECT_EQ(message.mdata[2], 0x38);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(1);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x3E);
    EXPECT_EQ(message.mdata[2], 0x38);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(2);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x6C);
    EXPECT_EQ(message.mdata[2], 0x60);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(3);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x36);
    EXPECT_EQ(message.mdata[2], 0x38);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(4);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x1C);
    EXPECT_EQ(message.mdata[2], 0x41);
    EXPECT_EQ(message.mdata[3], 0x00);

    message = this->canMsgSentLog.at(5);
    EXPECT_EQ(message.canId, 0x601);
    EXPECT_EQ(message.length, 8);
    EXPECT_EQ(message.mdata[0], 0x40);
    EXPECT_EQ(message.mdata[1], 0x37);
    EXPECT_EQ(message.mdata[2], 0x38);
    EXPECT_EQ(message.mdata[3], 0x00);

    EXPECT_EQ(canOpenState.pendingSdoRequests->first, nullptr);

    EXPECT_FLOAT_EQ(nodes[0].device->voltage->variant.value.Float, 52.5F);
    EXPECT_FLOAT_EQ(nodes[0].device->current->variant.value.Float, 10.0F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, 525);
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.SN16, 25);
    EXPECT_FLOAT_EQ(nodes[0].device->motorTorque->variant.value.Float, 80.0F);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.SN16, 30);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

// Battery current is signed, so regenerative braking gives a negative current
// and a negative power.
TEST_F(DmcTest, regenerativeBraking) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage, 525 = 52.5V
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3F, 0x38, 0x00, 0x0D, 0x02, 0x00, 0x00}});
    // Battery Current, -225 = -22.5A
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3E, 0x38, 0x00, 0x1F, 0xFF, 0x00, 0x00}});
    // Motor RPM, 500
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature, 76 = 25°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x36, 0x38, 0x00, 0x4C, 0x00, 0x00, 0x00}});
    // Motor Torque, -800 = -80.0Nm
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x1C, 0x41, 0x00, 0xE0, 0xFC, 0x00, 0x00}});
    // Controller Temperature, 81 = 30°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x37, 0x38, 0x00, 0x51, 0x00, 0x00, 0x00}});

    // Fault code 0, no fault
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x40, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Fault subcode 0
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x41, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});

    drainQueue(queue);

    EXPECT_FLOAT_EQ(nodes[0].device->current->variant.value.Float, -22.5F);
    EXPECT_EQ(nodes[0].device->power->variant.value.SN32, -1181);
    EXPECT_FLOAT_EQ(nodes[0].device->motorTorque->variant.value.Float, 80.0F);
}

// Temperatures are reported as an unsigned byte with a fixed offset of -51,
// so a raw value below 51 is a temperature below zero.
TEST_F(DmcTest, subZeroTemperatures) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);

    std::vector<VeRawCanMsg> queue;
    // Battery Voltage, 525 = 52.5V
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3F, 0x38, 0x00, 0x0D, 0x02, 0x00, 0x00}});
    // Battery Current, 100 = 10.0A
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3E, 0x38, 0x00, 0x64, 0x00, 0x00, 0x00}});
    // Motor RPM, 500
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    // Motor Temperature, 41 = -10°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x36, 0x38, 0x00, 0x29, 0x00, 0x00, 0x00}});
    // Motor Torque, 0
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x1C, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Controller Temperature, 0 = -51°C
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x37, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});

    // Fault code 0, no fault
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x40, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Fault subcode 0
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x41, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});

    drainQueue(queue);

    EXPECT_EQ(nodes[0].device->motorTemperature->variant.value.SN16, -10);
    EXPECT_EQ(nodes[0].device->controllerTemperature->variant.value.SN16, -51);
}

TEST_F(DmcTest, skipResponseOnDisconnect) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);

    std::vector<VeRawCanMsg> queue;
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3F, 0x38, 0x00, 0x0D, 0x02, 0x00, 0x00}});
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x3E, 0x38, 0x00, 0x64, 0x00, 0x00, 0x00}});
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x36, 0x38, 0x00, 0x4C, 0x00, 0x00, 0x00}});
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x1C, 0x41, 0x00, 0x20, 0x03, 0x00, 0x00}});
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x37, 0x38, 0x00, 0x51, 0x00, 0x00, 0x00}});

    disconnectFromNode(1);

    // Fault code 0, no fault
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4F, 0x40, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});
    // Fault subcode 0
    queue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x4B, 0x41, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00}});

    drainQueue(queue);

    EXPECT_EQ(this->canMsgSentLog.size(), 8);

    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
}

// Every request in the read routine times out, so onError disconnects the
// node on the first one and returns early for the rest.
TEST_F(DmcTest, readErrorTimeout) {
    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);

    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(this->canMsgSentLog.size(), 8);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
    EXPECT_EQ(nodes[0].connected, veFalse);
}

// Motor RPM comes from the DS402 object 0x606C, which is a signed 32 bit
// value, so a negative speed is sign extended across all four data bytes.
TEST_F(DmcTest, motorDirection) {
    VeVariant v;

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    // 500 rpm, direction not inverted
    nodes[0].device->driver->fastReadRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 1);
    this->canMsgReadQueue.push_back(
        {.canId = 0x581,
         .length = 8,
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
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
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0x0C, 0xFE, 0xFF, 0xFF}});
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
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}});
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
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0xF4, 0x01, 0x00, 0x00}});
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
         .mdata = {0x43, 0x6C, 0x60, 0x00, 0x0C, 0xFE, 0xFF, 0xFF}});
    canOpenTx();
    canOpenRx();
    EXPECT_EQ(nodes[0].device->motorRpm->variant.value.UN16, 500);
    EXPECT_EQ(nodes[0].device->motorDirection->variant.value.UN8, 2);
}

// EMCY payload: bytes 0-1 emergency error code, byte 2 error register,
// byte 3 DMC fault code, bytes 4-7 DMC fault subcode.
// Faults are polled because EMCY alone is not enough: it is edge triggered, so
// a fault standing when the driver starts is never seen. On a bench pair,
// provoking F13 with the throttle held forward at power up put F13 S2 in
// 0x3840 and 0x3841 for 23 seconds while 0x081 stayed silent throughout.
TEST_F(DmcTest, polledFault) {
    connectToDmcNode();
    this->canMsgSentLog.clear();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);

    // No fault, nothing reported.
    readCycleWithFault(0, 0);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);

    // F13 S2, accelerator more than 50% at power up.
    readCycleWithFault(13, 2);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
    EXPECT_EQ(injectPlatformNotification_fake.arg0_val,
              NOTIFICATION_TYPE_ERROR);
    EXPECT_STREQ(lastTitle, "Accelerator more than 50% at power up (F13 S2)");

    // The same fault still standing must not be announced again.
    readCycleWithFault(13, 2);
    readCycleWithFault(13, 2);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);

    // A different subcode is a different fault.
    readCycleWithFault(13, 3);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 2);
    EXPECT_STREQ(lastTitle, "Accelerator more than 50% at power up (F13 S3)");

    // Cleared, and then raised again, which must report a second time.
    readCycleWithFault(0, 0);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 2);
    readCycleWithFault(13, 2);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 3);

    // An unknown code still reports, with the raw numbers.
    readCycleWithFault(200, 7);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 4);
    EXPECT_STREQ(lastTitle, "Unknown fault (F200 S7)");
}

// No Sigma2N EMCY has been captured yet, so the EMCY frames below are written
// from the manual and CiA 301. Byte 2 is the error register: bit 0 generic,
// set for any error, bit 2 voltage, bit 3 temperature. The driver does not
// read it.
TEST_F(DmcTest, emcyMessage) {
    canOpenRegisterEmcyHandler(nodesEmcyHandler, NULL);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);

    EXPECT_EQ(nodes[0].connected, veFalse);
    connectToDmcNode();
    EXPECT_EQ(nodes[0].connected, veTrue);

    this->canMsgSentLog.clear();

    // Emergency error code 0, the fault was cleared
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);

    // F5 Motor temperature high, temperature error
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x40, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
    EXPECT_EQ(injectPlatformNotification_fake.arg0_val,
              NOTIFICATION_TYPE_ERROR);
    EXPECT_STREQ(lastTitle, "Motor temperature high (F5 S0)");

    // Exact same fault doesn't trigger a new notification
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x40, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);

    // Same fault code, but different subcode triggers a new notification
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x40, 0x09, 0x05, 0x01, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 2);
    EXPECT_EQ(injectPlatformNotification_fake.arg0_val,
              NOTIFICATION_TYPE_ERROR);
    EXPECT_STREQ(lastTitle, "Motor temperature high (F5 S1)");

    // F17 S2 Battery voltage too low, voltage error
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x30, 0x05, 0x11, 0x02, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 3);
    EXPECT_STREQ(lastTitle, "Battery voltage too low (F17 S2)");

    // A fault code that is not in the error database
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x10, 0x01, 0xC8, 0x07, 0x00, 0x00, 0x00}});
    canOpenRx();

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();

    EXPECT_EQ(injectPlatformNotification_fake.call_count, 4);
    EXPECT_STREQ(lastTitle, "Unknown fault (F200 S7)");
}

// A fault that clears and comes back between two polls is only visible
// through EMCY. The reset (error code 0) must re-arm the deduplication,
// otherwise the recurrence matches the cached fault and is dropped, and the
// next poll, which sees the same fault standing, drops it too.
TEST_F(DmcTest, emcyResetRearmsFault) {
    canOpenRegisterEmcyHandler(nodesEmcyHandler, NULL);
    connectToDmcNode();
    this->canMsgSentLog.clear();

    // F5 S0 Motor temperature high, temperature error
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x40, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);

    // The fault clears
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);

    // and comes back before any poll observed the clear
    this->canMsgReadQueue.push_back(
        {.canId = 0x081,
         .length = 8,
         .mdata = {0x00, 0x40, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00}});
    canOpenRx();
    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 2);
    EXPECT_STREQ(lastTitle, "Motor temperature high (F5 S0)");

    // The poll then sees the same fault standing, which is not new.
    readCycleWithFault(5, 0);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 2);
}

// A 6-phase motor driven by two controllers: the master at node 1 and the slave
// following its torque at node 2. Values are those of the bench pair under
// load, rounded.
class DmcDualMotorTest : public DmcTest {
  protected:
    static VeRawCanMsg response(un8 nodeId, un8 command, un16 index,
                                un32 data) {
        return {.canId = (un32)(0x580 + nodeId),
                .length = 8,
                .mdata = {command, (un8)(index & 0xFF), (un8)(index >> 8), 0x00,
                          (un8)(data & 0xFF), (un8)((data >> 8) & 0xFF),
                          (un8)((data >> 16) & 0xFF), (un8)(data >> 24)}};
    }

    void setSlaveNodeId(sn32 slaveNodeId) {
        VeVariant v;
        DmcContext *context;

        context = (DmcContext *)nodes[0].device->driverContext;
        veItemOwnerSet(context->slaveNodeIdSetting,
                       veVariantSn32(&v, slaveNodeId));
    }

    // 51.8 V, 13.3 A, 1408 RPM, motor 30 C, 20.0 Nm, controller 28 C
    void pushMasterReadings(std::vector<VeRawCanMsg> &queue) {
        queue.push_back(response(1, 0x4B, 0x383F, 518));
        queue.push_back(response(1, 0x4B, 0x383E, 133));
        queue.push_back(response(1, 0x43, 0x606C, 1408));
        queue.push_back(response(1, 0x4F, 0x3836, 30 + 51));
        queue.push_back(response(1, 0x4B, 0x411C, 200));
        queue.push_back(response(1, 0x4F, 0x3837, 28 + 51));
        queue.push_back(response(1, 0x4F, 0x3840, 0));
        queue.push_back(response(1, 0x4B, 0x3841, 0));
    }

    // 14.9 A, 18.5 Nm, motor 33 C, controller 26 C
    void pushSlaveReadings(std::vector<VeRawCanMsg> &queue, un8 faultCode = 0,
                           un16 faultSubcode = 0) {
        queue.push_back(response(2, 0x4B, 0x383E, 149));
        queue.push_back(response(2, 0x4B, 0x411C, 185));
        queue.push_back(response(2, 0x4F, 0x3836, 33 + 51));
        queue.push_back(response(2, 0x4F, 0x3837, 26 + 51));
        queue.push_back(response(2, 0x4F, 0x3840, faultCode));
        queue.push_back(response(2, 0x4B, 0x3841, faultSubcode));
    }

    void readCombinedCycle(un8 slaveFaultCode = 0, un16 slaveFaultSubcode = 0) {
        std::vector<VeRawCanMsg> queue;

        nodes[0].device->driver->readRoutine(&nodes[0]);
        EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 14);
        pushMasterReadings(queue);
        pushSlaveReadings(queue, slaveFaultCode, slaveFaultSubcode);
        drainQueue(queue);
        pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
        processPendingNotifications();
    }

    static veBool isValid(VeItem *item) {
        VeVariant v;

        veItemLocalValue(item, &v);
        return veVariantIsValid(&v);
    }
};

TEST_F(DmcDualMotorTest, combinesMasterAndSlave) {
    Device *device;

    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);
    this->canMsgSentLog.clear();

    readCombinedCycle();

    // The slave is read after the master, at its own node.
    EXPECT_EQ(this->canMsgSentLog.size(), 14);
    EXPECT_EQ(this->canMsgSentLog.at(7).canId, 0x601);
    EXPECT_EQ(this->canMsgSentLog.at(8).canId, 0x602);
    EXPECT_EQ(this->canMsgSentLog.at(13).canId, 0x602);

    device = nodes[0].device;
    EXPECT_FLOAT_EQ(device->voltage->variant.value.Float, 51.8F);
    EXPECT_FLOAT_EQ(device->current->variant.value.Float, 28.2F);
    EXPECT_EQ(device->power->variant.value.SN32, 1460);
    EXPECT_FLOAT_EQ(device->motorTorque->variant.value.Float, 38.5F);
    EXPECT_EQ(device->motorRpm->variant.value.UN16, 1408);
    EXPECT_EQ(device->motorDirection->variant.value.UN8, 2);
    EXPECT_EQ(device->motorTemperature->variant.value.SN16, 33);
    EXPECT_EQ(device->controllerTemperature->variant.value.SN16, 28);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);
}

TEST_F(DmcDualMotorTest, pairsWithTheOnlySlaveDiscovered) {
    dmcRecordDualMotorConfiguration(2, DMC_DUAL_MOTOR_SLAVE);
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(0);

    readCombinedCycle();

    EXPECT_FLOAT_EQ(nodes[0].device->current->variant.value.Float, 28.2F);

    // Kept, because after a restart the slave is not discovered again.
    EXPECT_EQ(((DmcContext *)nodes[0].device->driverContext)
                  ->slaveNodeIdSetting->variant.value.SN32,
              2);
}

// On a GX the write goes to localsettings over D-Bus, so the next cycle can
// still read 0 and find the same slave. It is not written a second time. The
// fake applies writes at once, so the setting is put back to 0 by hand.
TEST_F(DmcDualMotorTest, savesTheSlaveOnce) {
    dmcRecordDualMotorConfiguration(2, DMC_DUAL_MOTOR_SLAVE);
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(0);

    readCombinedCycle();
    setSlaveNodeId(0);

    readCombinedCycle();
    EXPECT_FLOAT_EQ(nodes[0].device->current->variant.value.Float, 28.2F);
    EXPECT_EQ(((DmcContext *)nodes[0].device->driverContext)
                  ->slaveNodeIdSetting->variant.value.SN32,
              0);
}

TEST_F(DmcDualMotorTest, doesNotGuessBetweenTwoSlaves) {
    dmcRecordDualMotorConfiguration(2, DMC_DUAL_MOTOR_SLAVE);
    dmcRecordDualMotorConfiguration(3, DMC_DUAL_MOTOR_SLAVE);
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(0);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);
}

TEST_F(DmcDualMotorTest, singleMotorDoesNotPair) {
    dmcRecordDualMotorConfiguration(2, DMC_DUAL_MOTOR_SLAVE);
    connectToDmcNode(0);
    setSlaveNodeId(0);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);
}

// Until localsettings has answered, the setting may hold a slave chosen
// earlier, so nothing is decided yet.
TEST_F(DmcDualMotorTest, waitsForTheSetting) {
    dmcRecordDualMotorConfiguration(2, DMC_DUAL_MOTOR_SLAVE);
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);
}

TEST_F(DmcDualMotorTest, ignoresItsOwnNodeId) {
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(1);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);
}

// The setting declares 0 to 127, but nothing guarantees localsettings holds to
// that, and the value becomes a CAN identifier.
TEST_F(DmcDualMotorTest, ignoresAnOutOfRangeSlaveNodeId) {
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);

    setSlaveNodeId(-1);
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 8);

    setSlaveNodeId(128);
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 16);
}

// Half the current, power and torque would read as the whole drive, so they
// are withheld while the slave is missing. The master stays connected, keeps
// its speed, voltage and own temperatures, and probes the slave with a single
// read per cycle until it answers again.
TEST_F(DmcDualMotorTest, slaveMissing) {
    Device *device;
    std::vector<VeRawCanMsg> queue;

    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);
    device = nodes[0].device;

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 14);
    pushMasterReadings(queue);
    drainQueue(queue);
    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(nodes[0].connected, veTrue);
    EXPECT_FALSE(isValid(device->current));
    EXPECT_FALSE(isValid(device->power));
    EXPECT_FALSE(isValid(device->motorTorque));
    EXPECT_FLOAT_EQ(device->voltage->variant.value.Float, 51.8F);
    EXPECT_EQ(device->motorRpm->variant.value.UN16, 1408);
    EXPECT_EQ(device->motorTemperature->variant.value.SN16, 30);
    EXPECT_EQ(device->controllerTemperature->variant.value.SN16, 28);

    // One warning, naming the slave.
    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
    EXPECT_EQ(injectPlatformNotification_fake.arg0_val,
              NOTIFICATION_TYPE_WARNING);
    EXPECT_STREQ(lastTitle, "Slave controller not responding");
    EXPECT_NE(strstr(lastDescription, "slave node 2"), nullptr);

    // Still missing: the master is read as before, the slave probed once.
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 9);
    pushMasterReadings(queue);
    drainQueue(queue);
    canOpenTx();
    pltGetCount1ms_fake.return_val += 50;
    canOpenTx();
    EXPECT_FALSE(isValid(device->current));
    EXPECT_EQ(device->motorTemperature->variant.value.SN16, 30);

    // The probe answers, and the next cycle reads the pair again.
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 9);
    pushMasterReadings(queue);
    queue.push_back(response(2, 0x4B, 0x383E, 149));
    drainQueue(queue);

    readCombinedCycle();
    EXPECT_FLOAT_EQ(device->current->variant.value.Float, 28.2F);
    EXPECT_EQ(device->power->variant.value.SN32, 1460);
    EXPECT_EQ(device->motorTemperature->variant.value.SN16, 33);

    // Not announced again while it stayed away, nor on its return.
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
}

// A slave fault is reported on the master's device, the only one the user
// sees, and names the slave.
TEST_F(DmcDualMotorTest, slaveFault) {
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);

    readCombinedCycle(13, 2);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
    EXPECT_EQ(injectPlatformNotification_fake.arg0_val,
              NOTIFICATION_TYPE_ERROR);
    EXPECT_STREQ(lastTitle, "Accelerator more than 50% at power up (F13 S2)");
    EXPECT_NE(strstr(lastDescription, "slave node 2"), nullptr);

    // Standing, not repeated.
    readCombinedCycle(13, 2);
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 1);
}

// The master losing its connection takes the drive with it, as for a single
// controller, and the slave's reads still queued are dropped.
TEST_F(DmcDualMotorTest, masterMissing) {
    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 14);
    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }

    EXPECT_EQ(nodes[0].connected, veFalse);
    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);
}

// Disconnecting does not empty the request queue, so the slave can still
// answer reads queued by a master that has just been dropped. Its answers are
// ignored.
TEST_F(DmcDualMotorTest, masterLostMidCycle) {
    std::vector<VeRawCanMsg> queue;

    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);

    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 14);

    // The master answers all but its last read, 0x3841, which times out.
    pushMasterReadings(queue);
    queue.pop_back();
    drainQueue(queue);
    canOpenTx();
    pltGetCount1ms_fake.return_val += 50;
    canOpenTx();
    EXPECT_EQ(nodes[0].connected, veFalse);

    pushSlaveReadings(queue, 13, 2);
    drainQueue(queue);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);

    pltGetCount1ms_fake.return_val += NOTIFICATION_INJECTION_DELAY_MS;
    processPendingNotifications();
    EXPECT_EQ(injectPlatformNotification_fake.call_count, 0);
}

// The same while the slave is missing, when the cycle only probes it.
TEST_F(DmcDualMotorTest, masterLostWhileProbing) {
    std::vector<VeRawCanMsg> queue;

    connectToDmcNode(DMC_DUAL_MOTOR_MASTER);
    setSlaveNodeId(2);

    // The slave does not answer, so it is marked missing.
    nodes[0].device->driver->readRoutine(&nodes[0]);
    pushMasterReadings(queue);
    drainQueue(queue);
    while (listCount(canOpenState.pendingSdoRequests) > 0) {
        canOpenTx();
        pltGetCount1ms_fake.return_val += 50;
        canOpenTx();
    }
    EXPECT_EQ(nodes[0].connected, veTrue);

    // Next cycle: the master's 0x3841 times out, then the probe answers.
    nodes[0].device->driver->readRoutine(&nodes[0]);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 9);
    pushMasterReadings(queue);
    queue.pop_back();
    drainQueue(queue);
    canOpenTx();
    pltGetCount1ms_fake.return_val += 50;
    canOpenTx();
    EXPECT_EQ(nodes[0].connected, veFalse);

    queue.push_back(response(2, 0x4B, 0x383E, 149));
    drainQueue(queue);
    EXPECT_EQ(listCount(canOpenState.pendingSdoRequests), 0);
}
