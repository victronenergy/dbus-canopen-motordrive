#ifndef __CANOPEN_H__
#define __CANOPEN_H__

#include <velib/canhw/canhw.h>

#define SDO_COMMAND_MASK                0b11100000
#define SDO_EXPEDITED                   0b00000010
#define SDO_ABORT_CONTROL               0b10000000
#define SDO_ABORT_TIMEOUT               0x05040000
#define SDO_ABORT_OUT_OF_MEMORY         0x05040005
#define SDO_READ_REQUEST_CONTROL        0b01000000
#define SDO_READ_RESPONSE_CONTROL       0b01000000
#define SDO_WRITE_REQUEST_CONTROL       0b00100000
#define SDO_WRITE_RESPONSE_CONTROL      0b01100000
#define SDO_ERROR_TIMEOUT               0x0a
#define SDO_READ_ERROR                  0x02
#define SDO_READ_ERROR_TIMEOUT          0x03
#define SDO_READ_ERROR_SEGMENT_TRANSFER 0x04
#define SDO_WRITE_ERROR                 0x08

#define MAX_SDO_SEND_TRIES 3

#pragma pack(1)
typedef union
{
    un8 byte[8];

    struct
    {
        un8 control;
        un16 index;
        un8 subindex;
        un32 data;
    };
} SdoMessage;

void logRawCanMessage(const VeRawCanMsg *message);
void logSdoMessage(const SdoMessage *message);

un8 readSdo(un8 nodeId, un32 index, un8 subindex, SdoMessage *response);
un8 writeSdo(un8 nodeId, un32 index, un8 subindex, un32 data);

#endif