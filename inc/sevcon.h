#ifndef __SEVCON_H__
#define __SEVCON_H__

#include <velib/base/types.h>

veBool sevconLogin(un8 nodeId);
veBool sevconFetchBatteryVoltage(un8 nodeId, float *voltage);
veBool sevconFetchBatteryCurrent(un8 nodeId, float *current);
veBool sevconFetchEngineRpm(un8 nodeId, un32 *rpm);
veBool sevconFetchEngineTemperature(un8 nodeId, un16 *temperature);
veBool sevconFetchSerialNumber(un8 nodeId, un32 *serialNumber);

#endif