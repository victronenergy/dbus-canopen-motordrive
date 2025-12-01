#pragma once

#include <gtest/gtest.h>
#include "environment.hpp"

class BaseFixture : public ::testing::Test {
protected:
    void SetUp() override {
        RESET_FAKE(_malloc);
        RESET_FAKE(_realloc);
        RESET_FAKE(_free);

        _malloc_fake.custom_fake = malloc;
        _realloc_fake.custom_fake = realloc;
        _free_fake.custom_fake = free;

        RESET_FAKE(veItemSet);
    }

    void TearDown() override {

    }
};