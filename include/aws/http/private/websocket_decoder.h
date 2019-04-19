#ifndef AWS_HTTP_WEBSOCKET_DECODER_H
#define AWS_HTTP_WEBSOCKET_DECODER_H

/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/http/http.h>

enum aws_websocket_opcode {
    AWS_WEBSOCKET_OPCODE_CONTINUATION = 0x0,
    AWS_WEBSOCKET_OPCODE_TEXT = 0x1,
    AWS_WEBSOCKET_OPCODE_DATA = 0x2,
    AWS_WEBSOCKET_OPCODE_CLOSE = 0x8,
    AWS_WEBSOCKET_OPCODE_PING = 0x9,
    AWS_WEBSOCKET_OPCODE_PONG = 0xA,
};

/**
 * Full contents of a websocket frame, excluding the payload.
 */
struct aws_websocket_frame {
    bool fin;
    bool rsv[3];
    bool masked;
    uint8_t opcode;
    uint64_t payload_length;
    uint8_t masking_key[4];
};

/* Called when the non-payload portion of a frame has been decoded. */
typedef int(aws_websocket_decoder_frame_fn)(const struct aws_websocket_frame *frame, void *user_data);

/* Called repeatedly as the payload is decoded. If a mask was used, the data has been unmasked. */
typedef int(aws_websocket_decoder_payload_fn)(struct aws_byte_cursor data, void *user_data);

/**
 * Each state consumes data and/or moves decoder to a subsequent state.
 */
enum aws_websocket_decoder_state {
    AWS_WEBSOCKET_DECODER_STATE_INIT,
    AWS_WEBSOCKET_DECODER_STATE_OPCODE_BYTE,
    AWS_WEBSOCKET_DECODER_STATE_LENGTH_BYTE,
    AWS_WEBSOCKET_DECODER_STATE_EXTENDED_LENGTH,
    AWS_WEBSOCKET_DECODER_STATE_MASKING_KEY_CHECK,
    AWS_WEBSOCKET_DECODER_STATE_MASKING_KEY,
    AWS_WEBSOCKET_DECODER_STATE_PAYLOAD_CHECK,
    AWS_WEBSOCKET_DECODER_STATE_PAYLOAD,
    AWS_WEBSOCKET_DECODER_STATE_DONE,
};

struct aws_websocket_decoder {
    enum aws_websocket_decoder_state state;
    uint64_t state_bytes_processed; /* For multi-byte states, the number of bytes processed so far */
    uint8_t state_cache[8];         /* For multi-byte states to cache data that might be split across packets */

    struct aws_websocket_frame current_frame; /* Data about current frame being decoded */

    bool expecting_continuation_data_frames; /* True when the next data frame must be CONTINUATION frame */

    void *user_data;
    aws_websocket_decoder_frame_fn *on_frame;
    aws_websocket_decoder_payload_fn *on_payload;
};

/**
 * Return true if opcode is for a data frame, false if opcode if for a control frame.
 */
AWS_STATIC_IMPL
bool aws_websocket_is_data_frame(uint8_t opcode) {
    return !(opcode & 0x08); /* RFC-6455 Section 5.6: Most significant bit of (4 bit) data frame opcode is 0 */
}

AWS_EXTERN_C_BEGIN

AWS_HTTP_API
void aws_websocket_decoder_init(
    struct aws_websocket_decoder *decoder,
    aws_websocket_decoder_frame_fn *on_frame,
    aws_websocket_decoder_payload_fn *on_payload,
    void *user_data);

/**
 * Returns when all data is processed, or a frame and its payload have completed.
 * `data` will be advanced to reflect the amount of data processed by this call.
 * `frame_complete` will be set true if this call returned due to completion of a frame.
 * The `on_frame` and `on_payload` callbacks may each be invoked once as a result of this call.
 * If an error occurs, the decoder is invalid forevermore.
 */
AWS_HTTP_API
int aws_websocket_decoder_process(
    struct aws_websocket_decoder *decoder,
    struct aws_byte_cursor *data,
    bool *frame_complete);

AWS_EXTERN_C_END

#endif /* AWS_HTTP_WEBSOCKET_DECODER_H */