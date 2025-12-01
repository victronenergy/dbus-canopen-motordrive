#pragma once

#include "environment.hpp"
#include <gtest/gtest.h>

static struct VeDbus fakeDbusInstance;
static struct VeRemoteService fakeRemoteService;

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
        RESET_FAKE(veDbusGetDefaultBus);
        veDbusGetDefaultBus_fake.return_val = &fakeDbusInstance;
        RESET_FAKE(veDbusGetVrmDeviceInstanceExt);
        veDbusGetVrmDeviceInstanceExt_fake.return_val = 9999;
        RESET_FAKE(veDbusChangeName);
        veDbusChangeName_fake.return_val = veTrue;
        RESET_FAKE(veCanSend);
        veCanSend_fake.return_val = veTrue;
        RESET_FAKE(veCanRead);
        veCanRead_fake.return_val = veTrue;
        RESET_FAKE(pltGetCount1ms);
        pltGetCount1ms_fake.return_val = 0;
        RESET_FAKE(veDbusAddRemoteService);
        veDbusAddRemoteService_fake.return_val = &fakeRemoteService;
    }

    void TearDown() override {}
};