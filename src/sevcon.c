#include <canopen.h>
#include <sevcon.h>

veBool sevconLogin(un8 nodeId) {
    SdoMessage response;

    if (readSdo(nodeId, 0x5000, 1, &response) != 0) {
        return veTrue;
    }

    if (response.data != 4) {
        // set user id
        if (writeSdo(nodeId, 0x5000, 3, 0) != 0) {
            return veTrue;
        }
        // set password
        if (writeSdo(nodeId, 0x5000, 2, 0x4bdf) != 0) {
            return veTrue;
        }

        if (readSdo(nodeId, 0x5000, 1, &response) != 0) {
            return veTrue;
        }

        if (response.data != 4) {
            return veTrue;
        }
    }

    return veFalse;
}

veBool sevconFetchBatteryVoltage(un8 nodeId, float *voltage) {
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 1, &response) != 0) {
        return veTrue;
    }
    *voltage = response.data * 0.0625F;
    return veFalse;
}

veBool sevconFetchBatteryCurrent(un8 nodeId, float *current) {
    SdoMessage response;
    if (readSdo(nodeId, 0x5100, 2, &response) != 0) {
        return veTrue;
    }
    *current = response.data * 0.0625F;
    return veFalse;
}

veBool sevconFetchEngineRpm(un8 nodeId, un32 *rpm) {
    SdoMessage response;
    if (readSdo(nodeId, 0x606c, 0, &response) != 0) {
        return veTrue;
    }
    *rpm = response.data;
    return veFalse;
}

veBool sevconFetchEngineTemperature(un8 nodeId, un16 *temperature) {
    SdoMessage response;
    if (readSdo(nodeId, 0x4600, 3, &response) != 0) {
        return veTrue;
    }
    *temperature = response.data;
    return veFalse;
}

veBool sevconFetchSerialNumber(un8 nodeId, un32 *serialNumber) {
    SdoMessage response;
    if (readSdo(nodeId, 0x1018, 4, &response) != 0) {
        return veTrue;
    }
    *serialNumber = response.data;
    return veFalse;
}