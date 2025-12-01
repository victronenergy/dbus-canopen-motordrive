#pragma once

#include <gtest/gtest.h>
#include "environment.hpp"

extern "C" {
    #include <localsettings.h>
}

static struct VeDbus fakeDbusInstance;

class BaseFixture : public ::testing::Test {
protected:
    void SetUp() override {
        RESET_FAKE(_malloc);
        RESET_FAKE(_realloc);
        RESET_FAKE(_free);

        _malloc_fake.custom_fake = malloc;
        _realloc_fake.custom_fake = realloc;
        _free_fake.custom_fake = free;

        RESET_FAKE(veDbusConnectString);
        veDbusConnectString_fake.return_val = &fakeDbusInstance;
        RESET_FAKE(veDbusGetVrmDeviceInstanceExt);
        veDbusGetVrmDeviceInstanceExt_fake.return_val = 9999;
        RESET_FAKE(veDbusChangeName);
        veDbusChangeName_fake.return_val = veTrue;

        localSettingsInit();
    }

    void TearDown() override {

    }
};