#include <canopen.h>
#include <logger.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <velib/utils/ve_timer.h>

CanOpenState canOpenState = {
    .pendingSdoRequests = NULL,
};

static void logRawCanMessage(const VeRawCanMsg *message) {
    info("canId=%X length=%d mdata=0x%02X%02X%02X%02X%02X%02X%02X%02X",
         message->canId, message->length, message->mdata[0], message->mdata[1],
         message->mdata[2], message->mdata[3], message->mdata[4],
         message->mdata[5], message->mdata[6], message->mdata[7]);
}

static void sendRawSdoRequest(un8 nodeId, const SdoMessage *request) {
    VeRawCanMsg message;
    message.canId = 0x600 + nodeId;
    message.length = 8;
    memcpy(message.mdata, request->byte, 8);
    message.flags = VE_CAN_STD;

    info("CAN_SEND");
    logRawCanMessage(&message);

    for (un8 tries = 0; tries < MAX_SDO_SEND_TRIES; tries += 1) {
        info("CAN_SEND_TRY_%d", tries);
        if (veCanSend(&message)) {
            info("CAN_SEND_SUCCESS");
            return;
        } else {
            warning("CAN_SEND_ERROR");
        }
    }
    warning("CAN_SEND_TOO_MANY_FAILURES");
}

void canOpenReadSdoAsync(un8 nodeId, un16 index, un8 subindex, void *context,
                         void (*onResponse)(CanOpenPendingSdoRequest *request),
                         void (*onError)(CanOpenPendingSdoRequest *request)) {
    CanOpenPendingSdoRequest *pendingRequest;

    pendingRequest = _malloc(sizeof(*pendingRequest));
    if (!pendingRequest) {
        error("Failed to allocate memory for CanOpenPendingSdoRequest");
        pltExit(5);
    }

    pendingRequest->nodeId = nodeId;
    pendingRequest->type = READ_SDO;
    pendingRequest->state = NOT_SENT;
    pendingRequest->index = index;
    pendingRequest->subindex = subindex;
    pendingRequest->onResponse = onResponse;
    pendingRequest->onError = onError;
    pendingRequest->timeout = 0;
    pendingRequest->context = context;
    pendingRequest->segmented_buffer = NULL;
    pendingRequest->segmented_length = NULL;
    pendingRequest->segmented_max_length = 0;
    pendingRequest->segmented_toggle = 0;

    info("SDO_READ_ASYNC nodeId=%d index=%X subindex=%X",
         pendingRequest->nodeId, pendingRequest->index,
         pendingRequest->subindex);

    listAdd(canOpenState.pendingSdoRequests, pendingRequest);
}

void canOpenQueueCallbackAsync(
    void *context, void (*callback)(CanOpenPendingSdoRequest *request)) {
    CanOpenPendingSdoRequest *pendingRequest;

    pendingRequest = _malloc(sizeof(*pendingRequest));
    if (!pendingRequest) {
        error("Failed to allocate memory for CanOpenPendingSdoRequest");
        pltExit(5);
    }

    pendingRequest->nodeId = 0;
    pendingRequest->type = QUEUE_CALLBACK;
    pendingRequest->state = NOT_SENT;
    pendingRequest->index = 0;
    pendingRequest->subindex = 0;
    pendingRequest->onResponse = callback;
    pendingRequest->onError = NULL;
    pendingRequest->timeout = 0;
    pendingRequest->context = context;
    pendingRequest->segmented_buffer = NULL;
    pendingRequest->segmented_length = NULL;
    pendingRequest->segmented_max_length = 0;
    pendingRequest->segmented_toggle = 0;

    listAdd(canOpenState.pendingSdoRequests, pendingRequest);
}

void canOpenReadSegmentedSdoAsync(
    un8 nodeId, un16 index, un8 subindex, void *context, un8 *buffer,
    un8 *length, un8 max_length,
    void (*onResponse)(CanOpenPendingSdoRequest *request),
    void (*onError)(CanOpenPendingSdoRequest *request)) {
    CanOpenPendingSdoRequest *pendingRequest;

    pendingRequest = _malloc(sizeof(*pendingRequest));
    if (!pendingRequest) {
        error("Failed to allocate memory for CanOpenPendingSdoRequest");
        pltExit(5);
    }

    pendingRequest->nodeId = nodeId;
    pendingRequest->type = READ_SEGMENTED_SDO;
    pendingRequest->state = NOT_SENT;
    pendingRequest->index = index;
    pendingRequest->subindex = subindex;
    pendingRequest->onResponse = onResponse;
    pendingRequest->onError = onError;
    pendingRequest->timeout = 0;
    pendingRequest->context = context;
    pendingRequest->segmented_buffer = buffer;
    pendingRequest->segmented_length = length;
    *pendingRequest->segmented_length = 0;
    pendingRequest->segmented_max_length = max_length;
    pendingRequest->segmented_toggle = 0;

    info("SDO_READ_SEGMENTED_ASYNC nodeId=%d index=%X subindex=%X",
         pendingRequest->nodeId, pendingRequest->index,
         pendingRequest->subindex);

    listAdd(canOpenState.pendingSdoRequests, pendingRequest);
}

void canOpenInit() { canOpenState.pendingSdoRequests = listCreate(); }

static void handleReadSdoResponse(ListItem *item,
                                  CanOpenPendingSdoRequest *pendingRequest) {
    SdoMessage abort_request;

    listRemove(canOpenState.pendingSdoRequests, item);

    if ((pendingRequest->response.control & SDO_COMMAND_MASK) !=
        SDO_READ_RESPONSE_CONTROL) {
        warning("SDO_READ_ERROR");
        pendingRequest->onError(pendingRequest);
    } else if (pendingRequest->response.control & SDO_EXPEDITED) {
        pendingRequest->onResponse(pendingRequest);
    } else {
        abort_request.control = SDO_ABORT_CONTROL;
        abort_request.data = SDO_ABORT_OUT_OF_MEMORY;
        abort_request.index = pendingRequest->index;
        abort_request.subindex = pendingRequest->subindex;
        warning("SDO_READ_ASYNC_ERROR_SEGMENT_TRANSFER");
        sendRawSdoRequest(pendingRequest->nodeId, &abort_request);
    }

    _free(pendingRequest);
}

static void
handleReadSegmentedSdoResponse(ListItem *item,
                               CanOpenPendingSdoRequest *pendingRequest) {
    SdoMessage request;
    un8 data_length;
    un8 bytes_to_copy;

    if (*pendingRequest->segmented_length == 0) {
        // First response, no data yet
        if ((pendingRequest->response.control & SDO_COMMAND_MASK) !=
            SDO_READ_RESPONSE_CONTROL) {
            warning("SDO_READ_ERROR");
            listRemove(canOpenState.pendingSdoRequests, item);
            pendingRequest->onError(pendingRequest);
            _free(pendingRequest);
            return;
        }

        if (pendingRequest->response.control & SDO_EXPEDITED) {
            // @todo: confirm that's how it work
            data_length = 4 - ((pendingRequest->response.control &
                                SDO_EXPEDITED_UNUSED_MASK) >>
                               2);
            un8 bytes_to_copy =
                data_length > pendingRequest->segmented_max_length
                    ? pendingRequest->segmented_max_length
                    : data_length;
            memcpy(pendingRequest->segmented_buffer,
                   &pendingRequest->response.data, bytes_to_copy);
            *pendingRequest->segmented_length = bytes_to_copy;

            listRemove(canOpenState.pendingSdoRequests, item);
            pendingRequest->onResponse(pendingRequest);
            _free(pendingRequest);

            return;
        }

        request.control = SDO_READ_SEGMENT_REQUEST;
        request.index = 0;
        request.subindex = 0;
        request.data = 0;

        pendingRequest->timeout = pltGetCount1ms();
        *pendingRequest->segmented_length = 1;

        sendRawSdoRequest(pendingRequest->nodeId, &request);
        return;
    }

    if (*pendingRequest->segmented_length == 1) {
        *pendingRequest->segmented_length = 0;
    }

    data_length =
        7 - ((pendingRequest->response.control & SDO_SEGMENT_UNUSED_MASK) >> 1);
    bytes_to_copy = *pendingRequest->segmented_length + data_length >
                            pendingRequest->segmented_max_length
                        ? pendingRequest->segmented_max_length -
                              *pendingRequest->segmented_length
                        : data_length;

    memcpy(pendingRequest->segmented_buffer + *pendingRequest->segmented_length,
           pendingRequest->response.byte + 1, bytes_to_copy);
    *pendingRequest->segmented_length += bytes_to_copy;

    if (pendingRequest->response.control & SDO_SEGMENT_END) {
        listRemove(canOpenState.pendingSdoRequests, item);
        pendingRequest->onResponse(pendingRequest);
        _free(pendingRequest);
        return;
    }

    if (*pendingRequest->segmented_length ==
        pendingRequest->segmented_max_length) {
        request.control = SDO_ABORT_CONTROL;
        request.data = SDO_ABORT_OUT_OF_MEMORY;
        request.index = pendingRequest->index;
        request.subindex = pendingRequest->subindex;

        sendRawSdoRequest(pendingRequest->nodeId, &request);

        warning("SDO_READ_ERROR_SEGMENT_MAX_LENGTH");
        listRemove(canOpenState.pendingSdoRequests, item);
        pendingRequest->onError(pendingRequest);
        _free(pendingRequest);
        return;
    }

    pendingRequest->segmented_toggle =
        pendingRequest->segmented_toggle ? 0 : SDO_SEGMENT_TOGGLE;
    pendingRequest->timeout = pltGetCount1ms();

    request.control =
        (SDO_READ_SEGMENT_REQUEST | pendingRequest->segmented_toggle);
    request.index = 0;
    request.subindex = 0;
    request.data = 0;

    sendRawSdoRequest(pendingRequest->nodeId, &request);
}

static void handleSdoResponse(ListItem *item,
                              CanOpenPendingSdoRequest *pendingRequest) {
    switch (pendingRequest->type) {
    case READ_SDO:
        handleReadSdoResponse(item, pendingRequest);
        break;
    case READ_SEGMENTED_SDO:
        handleReadSegmentedSdoResponse(item, pendingRequest);
        break;
    case QUEUE_CALLBACK:
    default:
        break;
    }
}

void canOpenRx() {
    VeRawCanMsg message;
    ListItem *iterator;
    CanOpenPendingSdoRequest *pendingRequest;
    SdoMessage response;

    while (veCanRead(&message)) {
        memcpy(response.byte, message.mdata, message.length);
        iterator = canOpenState.pendingSdoRequests->first;
        if (iterator) {
            pendingRequest = (CanOpenPendingSdoRequest *)iterator->data;
            if (message.canId == 0x580 + pendingRequest->nodeId) {
                info("CAN_RESPONSE");
                logRawCanMessage(&message);

                memcpy(pendingRequest->response.byte, response.byte,
                       sizeof(response));
                handleSdoResponse(iterator, pendingRequest);
            }
        }
    }
}

static void handleReadSdoRequest(ListItem *item,
                                 CanOpenPendingSdoRequest *pendingRequest) {
    SdoMessage request;

    request.control = SDO_READ_REQUEST_CONTROL;
    request.index = pendingRequest->index;
    request.subindex = pendingRequest->subindex;
    request.data = 0;

    sendRawSdoRequest(pendingRequest->nodeId, &request);
}

static void
handleReadSegmentedSdoRequest(ListItem *item,
                              CanOpenPendingSdoRequest *pendingRequest) {
    SdoMessage request;

    request.control = SDO_READ_REQUEST_CONTROL;
    request.index = pendingRequest->index;
    request.subindex = pendingRequest->subindex;
    request.data = 0;
    *pendingRequest->segmented_length = 0;

    sendRawSdoRequest(pendingRequest->nodeId, &request);
}

static void
handleQueueCallbackRequest(ListItem *item,
                           CanOpenPendingSdoRequest *pendingRequest) {
    pendingRequest->onResponse(pendingRequest);
}

static void handleSdoRequest(ListItem *item,
                             CanOpenPendingSdoRequest *pendingRequest) {
    switch (pendingRequest->type) {
    case READ_SDO:
        handleReadSdoRequest(item, pendingRequest);
        break;
    case READ_SEGMENTED_SDO:
        handleReadSegmentedSdoRequest(item, pendingRequest);
        break;
    case QUEUE_CALLBACK:
    default:
        handleQueueCallbackRequest(item, pendingRequest);
        break;
    }
    pendingRequest->state = SENT;
    pendingRequest->timeout = pltGetCount1ms();
}

void canOpenTx() {
    ListItem *iterator;
    CanOpenPendingSdoRequest *pendingRequest;

    iterator = canOpenState.pendingSdoRequests->first;

    if (iterator) {
        pendingRequest = (CanOpenPendingSdoRequest *)iterator->data;

        if (pendingRequest->state == NOT_SENT) {
            handleSdoRequest(iterator, pendingRequest);
        }

        if (veTick1ms(&pendingRequest->timeout, 50)) {
            warning("SDO_TIMEOUT");
            listRemove(canOpenState.pendingSdoRequests, iterator);
            pendingRequest->onError(pendingRequest);
            _free(pendingRequest);
        }

        if (pendingRequest->type == QUEUE_CALLBACK) {
            listRemove(canOpenState.pendingSdoRequests, iterator);
            _free(pendingRequest);
        }
    }
}