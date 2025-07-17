#include <canopen.h>
#include <logger.h>
#include <string.h>
#include <velib/utils/ve_timer.h>

void logRawCanMessage(const VeRawCanMsg *message) {
    info("canId=%X length=%d mdata=0x%02X%02X%02X%02X%02X%02X%02X%02X",
         message->canId, message->length, message->mdata[0], message->mdata[1],
         message->mdata[2], message->mdata[3], message->mdata[4],
         message->mdata[5], message->mdata[6], message->mdata[7]);
}

void logSdoMessage(const SdoMessage *message) {
    const un8 *data = (const un8 *)&message->data;
    info("control=%X index=%X subindex=%d data=0x%02X%02X%02X%02X",
         message->control, message->index, message->subindex, data[0], data[1],
         data[2], data[3]);
}

void sendRawSdoRequest(un8 nodeId, const SdoMessage *request) {
    VeRawCanMsg message;
    message.canId = 0x600 + nodeId;
    message.length = 8;
    memcpy(message.mdata, request->byte, 8);

    info("CAN_SEND");
    logRawCanMessage(&message);

    for (un8 tries = 0; tries < MAX_SDO_SEND_TRIES; tries += 1) {
        info("CAN_SEND_TRY_%d", tries);
        if (veCanSend(&message)) {
            info("CAN_SEND_SUCCESS");
            return;
        } else {
            error("CAN_SEND_ERROR");
        }
    }
    error("CAN_SEND_TOO_MANY_FAILURES");
}

veBool waitForSdoResponse(un8 nodeId, SdoMessage *response) {
    VeRawCanMsg message;

    while (veCanRead(&message)) {
        info("CAN_RESPONSE");
        logRawCanMessage(&message);
        if (message.canId == 0x580 + nodeId) {
            memcpy(response->byte, message.mdata, message.length);
            return veTrue;
        }
    }
    return veFalse;
}

un8 sendSdoRequest(un8 nodeId, const SdoMessage *request,
                   SdoMessage *response) {
    un8 tries = 0;
    un8 timeout;

    info("SDO_SEND");
    logSdoMessage(request);

    while (tries < 3) {
        info("SDO_SEND_TRY_%d", tries);

        sendRawSdoRequest(nodeId, request);

        timeout = 50;
        while (waitForSdoResponse(nodeId, response) != veTrue && timeout > 0) {
            timeout -= 1;
            veWait(1000);
        }

        if (timeout != 0) {
            return 0;
        }

        SdoMessage abort_request;
        abort_request.control = SDO_ABORT_CONTROL;
        abort_request.data = SDO_ABORT_TIMEOUT;
        abort_request.index = request->index;
        abort_request.subindex = request->subindex;
        error("SDO_SEND_ABORT_TIMEOUT");
        logSdoMessage(&abort_request);
        sendRawSdoRequest(nodeId, &abort_request);

        if (tries < 2) {
            veWait(10000);
        }
        tries += 1;
    }

    error("SDO_SEND_TIMEOUT");

    return SDO_ERROR_TIMEOUT;
}

un8 readSdo(un8 nodeId, un32 index, un8 subindex, SdoMessage *response) {
    SdoMessage request;
    request.control = SDO_READ_REQUEST_CONTROL;
    request.index = index;
    request.subindex = subindex;
    request.data = 0;

    info("SDO_READ nodeId=%d index=%X subindex%d", nodeId, index, subindex);

    if (sendSdoRequest(nodeId, &request, response) != 0) {
        error("SDO_READ_ERROR_TIMEOUT");
        return SDO_READ_ERROR_TIMEOUT;
    }

    if ((response->control & SDO_COMMAND_MASK) != SDO_READ_RESPONSE_CONTROL) {
        error("SDO_READ_ERROR");
        return SDO_READ_ERROR;
    }

    if (response->control & SDO_EXPEDITED) {
        info("SDO_READ_SUCCESS");

        return 0;
    }

    SdoMessage abort_request;
    abort_request.control = SDO_ABORT_CONTROL;
    abort_request.data = SDO_ABORT_OUT_OF_MEMORY;
    abort_request.index = request.index;
    abort_request.subindex = request.subindex;
    error("SDO_READ_ERROR_SEGMENT_TRANSFER");
    sendRawSdoRequest(nodeId, &abort_request);

    return SDO_READ_ERROR_SEGMENT_TRANSFER;
}

un8 writeSdo(un8 nodeId, un32 index, un8 subindex, un32 data) {
    SdoMessage response;
    SdoMessage request;
    request.control = SDO_WRITE_REQUEST_CONTROL | SDO_EXPEDITED;
    request.index = index;
    request.subindex = subindex;
    request.data = data;

    info("SDO_WRITE nodeId=%d index=%X subindex%d data=%d", nodeId, index,
         subindex);

    if (sendSdoRequest(nodeId, &request, &response) != 0) {
        error("SDO_WRITE_ERROR_TIMEOUT");
        return SDO_READ_ERROR_TIMEOUT;
    }

    if ((response.control & SDO_COMMAND_MASK) != SDO_WRITE_RESPONSE_CONTROL) {
        error("SDO_WRITE_ERROR");
        return SDO_WRITE_ERROR;
    }

    return 0;
}

un8 readSegmentedSdo(un8 nodeId, un32 index, un8 subindex, un8 *buffer,
                     un8 *length, un8 max_length) {
    SdoMessage response;
    SdoMessage abort_request;
    SdoMessage request;

    request.control = SDO_READ_REQUEST_CONTROL;
    request.index = index;
    request.subindex = subindex;
    request.data = 0;

    *length = 0;

    info("SDO_READ_SEGMENTED nodeId=%d index=%X subindex%d", nodeId, index,
         subindex);

    if (sendSdoRequest(nodeId, &request, &response) != 0) {
        error("SDO_READ_ERROR_TIMEOUT");
        return SDO_READ_ERROR_TIMEOUT;
    }

    if ((response.control & SDO_COMMAND_MASK) != SDO_READ_RESPONSE_CONTROL) {
        error("SDO_READ_ERROR");
        return SDO_READ_ERROR;
    }

    if (response.control & SDO_EXPEDITED) {
        un8 data_length =
            4 - ((response.control & SDO_EXPEDITED_UNUSED_MASK) >> 2);
        un8 bytes_to_copy = data_length > max_length ? max_length : data_length;
        memcpy(buffer, &response.data, bytes_to_copy);
        *length = bytes_to_copy;

        return 0;
    }

    un8 toggle = 0;

    while (1) {
        request.control = (SDO_READ_SEGMENT_REQUEST | toggle);
        request.index = 0;
        request.subindex = 0;
        request.data = 0;

        if (sendSdoRequest(nodeId, &request, &response) != 0) {
            error("SDO_READ_ERROR_TIMEOUT");
            return SDO_READ_ERROR_TIMEOUT;
        }

        if ((response.control & (SDO_COMMAND_MASK | SDO_SEGMENT_TOGGLE)) !=
            (SDO_READ_SEGMENT_RESPONSE | toggle)) {
            abort_request.control = SDO_ABORT_CONTROL;
            abort_request.data = SDO_ABORT_OUT_OF_MEMORY;
            abort_request.index = request.index;
            abort_request.subindex = request.subindex;

            sendRawSdoRequest(nodeId, &abort_request);
            error("SDO_READ_ERROR_SEGMENT_MISMATCH");
            return SDO_READ_ERROR_SEGMENT_MISMATCH;
        }

        un8 data_length =
            8 - ((response.control & SDO_SEGMENT_UNUSED_MASK) >> 1);
        un8 bytes_to_copy = *length + data_length > max_length
                                ? max_length - *length
                                : data_length;
        memcpy(buffer, response.byte + 1, bytes_to_copy);
        buffer += bytes_to_copy;
        *length += bytes_to_copy;

        if (response.control & SDO_SEGMENT_END) {
            return 0;
        }

        if (*length == max_length) {
            abort_request.control = SDO_ABORT_CONTROL;
            abort_request.data = SDO_ABORT_OUT_OF_MEMORY;
            abort_request.index = request.index;
            abort_request.subindex = request.subindex;

            sendRawSdoRequest(nodeId, &abort_request);

            error("SDO_READ_ERROR_SEGMENT_MAX_LENGTH");
            return SDO_READ_ERROR_SEGMENT_MAX_LENGTH;
        }

        toggle = toggle ? 0 : SDO_SEGMENT_TOGGLE;
    }
}