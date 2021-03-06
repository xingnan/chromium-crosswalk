// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_websocket_test_util.h"

#include "net/spdy/buffered_spdy_framer.h"
#include "net/spdy/spdy_http_utils.h"

namespace net {

static const int kDefaultAssociatedStreamId = 0;
static const bool kDefaultCompressed = false;
static const char* const kDefaultDataPointer = NULL;
static const uint32 kDefaultDataLength = 0;
static const char** const kDefaultExtraHeaders = NULL;
static const int kDefaultExtraHeaderCount = 0;

SpdyWebSocketTestUtil::SpdyWebSocketTestUtil(
    NextProto protocol) : SpdyTestUtil(protocol) {}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketSynStream(
    int stream_id,
    const char* path,
    const char* host,
    const char* origin) {
  const char* const kWebSocketHeaders[] = {
    GetWebSocketPathKey(), path,
    GetHostKey(), host,
    GetVersionKey(), "WebSocket/13",
    GetSchemeKey(), "ws",
    GetOriginKey(), origin
  };
  return ConstructSpdyControlFrame(/*extra_headers*/ NULL,
                                   /*extra_header_count*/ 0,
                                   /*compressed*/ false,
                                   stream_id,
                                   HIGHEST,
                                   SYN_STREAM,
                                   CONTROL_FLAG_NONE,
                                   kWebSocketHeaders,
                                   arraysize(kWebSocketHeaders),
                                   0);
}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketSynReply(
    int stream_id) {
  static const char* const kStandardWebSocketHeaders[] = {
    GetStatusKey(), "101"
  };
  return ConstructSpdyControlFrame(NULL,
                                   0,
                                   false,
                                   stream_id,
                                   LOWEST,
                                   SYN_REPLY,
                                   CONTROL_FLAG_NONE,
                                   kStandardWebSocketHeaders,
                                   arraysize(kStandardWebSocketHeaders),
                                   0);
}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketHandshakeRequestFrame(
    const char* const headers[],
    int header_count,
    SpdyStreamId stream_id,
    RequestPriority request_priority) {
  // SPDY SYN_STREAM control frame header.
  const SpdyHeaderInfo kSynStreamHeader = {
    SYN_STREAM,
    stream_id,
    kDefaultAssociatedStreamId,
    ConvertRequestPriorityToSpdyPriority(request_priority, 2),
    kSpdyCredentialSlotUnused,
    CONTROL_FLAG_NONE,
    kDefaultCompressed,
    RST_STREAM_INVALID,
    kDefaultDataPointer,
    kDefaultDataLength,
    DATA_FLAG_NONE
  };

  // Construct SPDY SYN_STREAM control frame.
  return ConstructSpdyFrame(
      kSynStreamHeader,
      kDefaultExtraHeaders,
      kDefaultExtraHeaderCount,
      headers,
      header_count);
}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketHandshakeResponseFrame(
    const char* const headers[],
    int header_count,
    SpdyStreamId stream_id,
    RequestPriority request_priority) {
  // SPDY SYN_REPLY control frame header.
  const SpdyHeaderInfo kSynReplyHeader = {
    SYN_REPLY,
    stream_id,
    kDefaultAssociatedStreamId,
    ConvertRequestPriorityToSpdyPriority(request_priority, 2),
    kSpdyCredentialSlotUnused,
    CONTROL_FLAG_NONE,
    kDefaultCompressed,
    RST_STREAM_INVALID,
    kDefaultDataPointer,
    kDefaultDataLength,
    DATA_FLAG_NONE
  };

  // Construct SPDY SYN_REPLY control frame.
  return ConstructSpdyFrame(
      kSynReplyHeader,
      kDefaultExtraHeaders,
      kDefaultExtraHeaderCount,
      headers,
      header_count);
}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketHeadersFrame(
    int stream_id,
    const char* length,
    bool fin) {
  static const char* const kHeaders[] = {
    GetOpcodeKey(), "1",  // text frame
    GetLengthKey(), length,
    GetFinKey(), fin ? "1" : "0"
  };
  return ConstructSpdyControlFrame(/*extra_headers*/ NULL,
                                   /*extra_header_count*/ 0,
                                   /*compression*/ false,
                                   stream_id,
                                   LOWEST,
                                   HEADERS,
                                   CONTROL_FLAG_NONE,
                                   kHeaders,
                                   arraysize(kHeaders),
                                   0);
}

SpdyFrame* SpdyWebSocketTestUtil::ConstructSpdyWebSocketDataFrame(
    const char* data,
    int len,
    SpdyStreamId stream_id,
    bool fin) {

  // Construct SPDY data frame.
  BufferedSpdyFramer framer(spdy_version(), false);
  return framer.CreateDataFrame(
      stream_id,
      data,
      len,
      fin ? DATA_FLAG_FIN : DATA_FLAG_NONE);
}

const char* SpdyWebSocketTestUtil::GetWebSocketPathKey() const {
  return is_spdy2() ? "path" : ":path";
}

const char* SpdyWebSocketTestUtil::GetOriginKey() const {
  return is_spdy2() ? "origin" : ":origin";
}

const char* SpdyWebSocketTestUtil::GetOpcodeKey() const {
  return is_spdy2() ? "opcode" : ":opcode";
}

const char* SpdyWebSocketTestUtil::GetLengthKey() const {
  return is_spdy2() ? "length" : ":length";
}

const char* SpdyWebSocketTestUtil::GetFinKey() const {
  return is_spdy2() ? "fin" : ":fin";
}

}  // namespace net
