#include "BaseFixture.hpp"

extern "C" {
#include <device.h>
#include <localsettings.h>
#include <velib/vecan/products.h>
}

class DeviceTest : public BaseFixture {
  protected:
    void SetUp() override {
        BaseFixture::SetUp();

        localSettingsInit();
    }

    void TearDown() override { BaseFixture::TearDown(); }
};

Driver dummyDriver = {
    .name = "DummyDriver",
    .productId = VE_PROD_ID_SEVCON_MOTORDRIVE,
    .readRoutine = NULL,
    .fastReadRoutine = NULL,
};

TEST_F(DeviceTest, init) {
    char debug[1024];
    VeItem *item;
    VeVariant v;

    Device device;
    un8 nodeId;
    const char *serialNumber = "12345678";

    nodeId = 5;
    device.driver = &dummyDriver;

    veDbusChangeName_fake.custom_fake =
        (veBool (*)(struct VeDbus *, char const *)) +
        [](struct VeDbus *dbus, char const *name) {
            EXPECT_STREQ(name, "com.victronenergy.motordrive.Fake_Gateway_"
                               "DummyDriver_12345678");
            return veTrue;
        };

    createDevice(&device, nodeId, serialNumber);

    EXPECT_EQ(device.nodeId, nodeId);
    EXPECT_STREQ(device.serialNumber, "12345678");
    EXPECT_STREQ(device.identifier, "Fake_Gateway_DummyDriver_12345678");
    EXPECT_EQ(device.deviceInstance, 9999);

    veItemUid(device.root, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678");

    item = veItemByUid(device.root, "DeviceInstance");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/DeviceInstance");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_UN32);
    EXPECT_EQ(v.value.UN32, device.deviceInstance);

    item = veItemByUid(device.root, "ProductId");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/ProductId");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_UN16);
    EXPECT_EQ(v.value.UN16, device.driver->productId);

    item = veItemByUid(device.root, "ProductName");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/ProductName");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "Sevcon motor controller");

    item = veItemByUid(device.root, "Connected");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Connected");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_UN32);
    EXPECT_EQ(v.value.UN32, 1);

    item = veItemByUid(device.root, "Mgmt/Connection");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Mgmt/Connection");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "VE.Can");

    item = veItemByUid(device.root, "Mgmt/ProcessName");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Mgmt/ProcessName");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "dbus-canopen-motordrive");

    item = veItemByUid(device.root, "Mgmt/ProcessVersion");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug,
                 "/Fake_Gateway_DummyDriver_12345678/Mgmt/ProcessVersion");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "test_version");

    item = veItemByUid(device.root, "Serial");
    veItemUid(item, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Serial");
    veItemLocalValue(item, &v);
    EXPECT_EQ(v.type.tp, VE_STR);
    EXPECT_STREQ((char *)v.value.Ptr, "12345678");

    veItemUid(device.voltage, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Dc/0/Voltage");
    veItemLocalValue(device.voltage, &v);
    EXPECT_EQ(v.type.tp, VE_FLOAT);
    EXPECT_EQ(v.value.Float, 0.0f);

    veItemUid(device.current, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Dc/0/Current");
    veItemLocalValue(device.current, &v);
    EXPECT_EQ(v.type.tp, VE_FLOAT);
    EXPECT_EQ(v.value.Float, 0.0f);

    veItemUid(device.power, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Dc/0/Power");
    veItemLocalValue(device.power, &v);
    EXPECT_EQ(v.type.tp, VE_SN32);
    EXPECT_EQ(v.value.SN32, 0.0f);

    veItemUid(device.motorRpm, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Motor/RPM");
    veItemLocalValue(device.motorRpm, &v);
    EXPECT_EQ(v.type.tp, VE_UN16);
    EXPECT_EQ(v.value.UN16, VE_INVALID_UN16);

    veItemUid(device.motorDirection, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Motor/Direction");
    veItemLocalValue(device.motorDirection, &v);
    EXPECT_EQ(v.type.tp, VE_UN8);
    EXPECT_EQ(v.value.UN16, VE_INVALID_UN8);

    veItemUid(device.motorTemperature, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Motor/Temperature");
    veItemLocalValue(device.motorTemperature, &v);
    EXPECT_EQ(v.type.tp, VE_SN16);
    EXPECT_EQ(v.value.SN16, VE_INVALID_SN16);

    veItemUid(device.motorTorque, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/Motor/Torque");
    veItemLocalValue(device.motorTorque, &v);
    EXPECT_EQ(v.type.tp, VE_UN16);
    EXPECT_EQ(v.value.UN16, VE_INVALID_UN16);

    veItemUid(device.controllerTemperature, debug, sizeof(debug));
    EXPECT_STREQ(debug,
                 "/Fake_Gateway_DummyDriver_12345678/Controller/Temperature");
    veItemLocalValue(device.controllerTemperature, &v);
    EXPECT_EQ(v.type.tp, VE_SN16);
    EXPECT_EQ(v.value.SN16, VE_INVALID_SN16);

    veItemUid(device.motorDirectionInverted, debug, sizeof(debug));
    EXPECT_STREQ(
        debug,
        "/Fake_Gateway_DummyDriver_12345678/Settings/Motor/DirectionInverted");
    veItemLocalValue(device.motorDirectionInverted, &v);
    EXPECT_EQ(v.type.tp, VE_SN32);

    veItemUid(device.customName, debug, sizeof(debug));
    EXPECT_STREQ(debug, "/Fake_Gateway_DummyDriver_12345678/CustomName");
    veItemLocalValue(device.customName, &v);
    EXPECT_EQ(v.type.tp, VE_STR);

    destroyDevice(&device);
}

TEST_F(DeviceTest, failureToConnectToDbus) {
    Device device;
    un8 nodeId;
    const char *serialNumber;

    nodeId = 5;
    serialNumber = "12345678";
    device.driver = &dummyDriver;

    veDbusConnectString_fake.return_val = NULL;

    ASSERT_EXIT(createDevice(&device, nodeId, serialNumber);
                , ::testing::ExitedWithCode(1), "");
}

TEST_F(DeviceTest, failureToGetVrmInstance) {
    Device device;
    un8 nodeId;
    const char *serialNumber;

    nodeId = 5;
    serialNumber = "12345678";
    device.driver = &dummyDriver;

    veDbusGetVrmDeviceInstanceExt_fake.return_val = -1;

    ASSERT_EXIT(createDevice(&device, nodeId, serialNumber);
                , ::testing::ExitedWithCode(2), "");
}

TEST_F(DeviceTest, failureToRegisterDbusServiceName) {
    Device device;
    un8 nodeId;
    const char *serialNumber;

    nodeId = 5;
    serialNumber = "12345678";
    device.driver = &dummyDriver;

    veDbusChangeName_fake.return_val = veFalse;

    ASSERT_EXIT(createDevice(&device, nodeId, serialNumber);
                , ::testing::ExitedWithCode(3), "");
}