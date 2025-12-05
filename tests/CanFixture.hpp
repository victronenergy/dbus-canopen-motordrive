#pragma once

#include "BaseFixture.hpp"

class CanFixture;

static CanFixture *globalCanFixture = nullptr;

static veBool veCanSendLocal(VeRawCanMsg *message);
static veBool veCanReadLocal(VeRawCanMsg *message);

class CanFixture : public BaseFixture {
  public:
    std::vector<VeRawCanMsg> canMsgSentLog;
    std::vector<VeRawCanMsg> canMsgReadQueue;

  protected:
    void SetUp() override {
        BaseFixture::SetUp();

        globalCanFixture = this;

        this->canMsgSentLog.clear();
        this->canMsgReadQueue.clear();

        veCanSend_fake.custom_fake = veCanSendLocal;
        veCanRead_fake.custom_fake = veCanReadLocal;
    }

    void TearDown() override { BaseFixture::TearDown(); }
};

static veBool veCanSendLocal(VeRawCanMsg *message) {
    globalCanFixture->canMsgSentLog.push_back(*message);
    return veTrue;
}

static veBool veCanReadLocal(VeRawCanMsg *message) {
    if (!globalCanFixture->canMsgReadQueue.empty()) {
        memcpy(message, &globalCanFixture->canMsgReadQueue.front(),
               sizeof(VeRawCanMsg));
        globalCanFixture->canMsgReadQueue.erase(
            globalCanFixture->canMsgReadQueue.begin());
        return veTrue;
    }
    return veFalse;
}