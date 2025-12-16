#ifndef __CANOPEN_H__
#define __CANOPEN_H__

#include <list.h>
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

#define MAX_SDO_SEND_TRIES 3

typedef struct _CanOpenState {
    List *pendingSdoRequests;
} CanOpenState;

extern CanOpenState canOpenState;

typedef union {
    un8 byte[8];

    struct __attribute__((packed)) {
        un8 control;
        un16 index;
        un8 subindex;
        un32 data;
    };
} SdoMessage;

typedef enum {
    READ_SDO,
    READ_SEGMENTED_SDO,
    QUEUE_CALLBACK,
} CanOpenSdoRequestType;

typedef enum {
    NOT_SENT,
    SENT,
} CanOpenSdoRequestState;

typedef enum {
    SDO_READ_ERROR,
    SDO_READ_ERROR_TIMEOUT,
    SDO_READ_ERROR_SEGMENT_TRANSFER,
    SDO_READ_ERROR_SEGMENT_MISMATCH,
    SDO_READ_ERROR_SEGMENT_MAX_LENGTH,
    SDO_WRITE_ERROR,
} CanOpenError;

typedef struct _CanOpenPendingSdoRequest CanOpenPendingSdoRequest;

typedef struct _CanOpenPendingSdoRequest {
    un8 nodeId;
    CanOpenSdoRequestType type;
    CanOpenSdoRequestState state;
    un16 index;
    un8 subindex;
    SdoMessage response;
    void (*onResponse)(CanOpenPendingSdoRequest *request);
    void (*onError)(CanOpenPendingSdoRequest *request, CanOpenError error);
    un16 timeout;

    void *context;
    un8 *segmented_buffer;
    un8 *segmented_length;
    un8 segmented_max_length;
    un8 segmented_toggle;
} CanOpenPendingSdoRequest;

void canOpenInit();
void canOpenRx();
void canOpenTx();

void canOpenReadSdoAsync(un8 nodeId, un16 index, un8 subindex, void *context,
                         void (*onResponse)(CanOpenPendingSdoRequest *request),
                         void (*onError)(CanOpenPendingSdoRequest *request,
                                         CanOpenError error));

void canOpenReadSegmentedSdoAsync(
    un8 nodeId, un16 index, un8 subindex, void *context, un8 *buffer,
    un8 *length, un8 max_length,
    void (*onResponse)(CanOpenPendingSdoRequest *request),
    void (*onError)(CanOpenPendingSdoRequest *request, CanOpenError error));

void canOpenQueueCallbackAsync(
    void *context, void (*callback)(CanOpenPendingSdoRequest *request));

#endif