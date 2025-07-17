#include <canopen.h>
#include <sevcon.h>

veBool isSevcon(un8 nodeId) {
    un8 buffer[255];
    un8 length;

    if (readSegmentedSdo(nodeId, 0x1008, 0, buffer, &length, 255) != 0) {
        return veFalse;
    }

    if (length >= 4 && buffer[0] == 'G' && buffer[1] == 'e' &&
        buffer[2] == 'n' && buffer[3] == '4') {
        return veTrue;
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

veBool sevconFetchEngineRpm(un8 nodeId, sn16 *rpm) {
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