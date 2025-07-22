#ifndef __CANOPEN_H__
#define __CANOPEN_H__

#include <velib/canhw/canhw.h>

#define SDO_COMMAND_MASK 0b11100000
#define SDO_EXPEDITED 0b00000010
#define SDO_EXPEDITED_UNUSED_MASK 0b00001100
#define SDO_ABORT_CONTROL 0b10000000
#define SDO_ABORT_TIMEOUT 0x05040000
#define SDO_ABORT_OUT_OF_MEMORY 0x05040005
#define SDO_READ_REQUEST_CONTROL 0b01000000
#define SDO_READ_RESPONSE_CONTROL 0b01000000
#define SDO_READ_SEGMENT_REQUEST 0b01100000
#define SDO_READ_SEGMENT_RESPONSE 0b00000000
#define SDO_WRITE_REQUEST_CONTROL 0b00100000
#define SDO_WRITE_RESPONSE_CONTROL 0b01100000
#define SDO_SEGMENT_TOGGLE 0b00010000
#define SDO_SEGMENT_UNUSED_MASK 0b00001110
#define SDO_SEGMENT_END 0b00000001
#define SDO_ERROR_TIMEOUT 0x0a
#define SDO_READ_ERROR 0x02
#define SDO_READ_ERROR_TIMEOUT 0x03
#define SDO_READ_ERROR_SEGMENT_TRANSFER 0x04
#define SDO_READ_ERROR_SEGMENT_MISMATCH 0x05
#define SDO_READ_ERROR_SEGMENT_MAX_LENGTH 0x06
#define SDO_WRITE_ERROR 0x08

#define MAX_SDO_SEND_TRIES 3

typedef union {
    un8 byte[8];

    struct __attribute__((packed)) {
        un8 control;
        un16 index;
        un8 subindex;
        un32 data;
    };
} SdoMessage;

void logRawCanMessage(const VeRawCanMsg *message);
void logSdoMessage(const SdoMessage *message);

un8 readSdo(un8 nodeId, un32 index, un8 subindex, SdoMessage *response);
un8 readSegmentedSdo(un8 nodeId, un32 index, un8 subindex, un8 *buffer,
                     un8 *length, un8 max_length);
un8 writeSdo(un8 nodeId, un32 index, un8 subindex, un32 data);

#endif