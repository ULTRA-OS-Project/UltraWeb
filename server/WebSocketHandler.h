// UltraWeb/server/WebSocketHandler.h
// RFC 6455 WebSocket protocol: handshake and frame codec
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Transport-agnostic protocol layer used by UltraWebServer's built-in
// backend for delta updates and dev-server live reload. Contains no
// sockets - callers feed received bytes in and write encoded bytes out.
// Self-contained SHA-1/Base64 (needed only for the handshake) to avoid
// an OpenSSL dependency.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

// Exposed for tests
std::vector<uint8_t> SHA1(const uint8_t* data, size_t size);
std::string Base64Encode(const uint8_t* data, size_t size);

class WebSocketHandler {
public:
    enum class OpCode : uint8_t {
        Continuation = 0x0,
        Text         = 0x1,
        Binary       = 0x2,
        Close        = 0x8,
        Ping         = 0x9,
        Pong         = 0xA
    };

    struct Frame {
        bool fin = true;
        OpCode opcode = OpCode::Text;
        std::vector<uint8_t> payload;   // unmasked
    };

    // ===== HANDSHAKE =====

    // Sec-WebSocket-Accept for a client's Sec-WebSocket-Key
    static std::string ComputeAcceptKey(const std::string& clientKey);

    // Extracts the Sec-WebSocket-Key from an HTTP upgrade request; false
    // when the request is not a well-formed WebSocket upgrade
    static bool ParseUpgradeRequest(const std::string& httpRequest,
                                    std::string& clientKey);

    // Complete "101 Switching Protocols" response for a client key
    static std::string BuildUpgradeResponse(const std::string& clientKey);

    // ===== FRAMES =====

    // Encodes a server-to-client frame (unmasked, per RFC 6455 5.1)
    static std::vector<uint8_t> EncodeFrame(OpCode opcode,
                                            const uint8_t* payload, size_t size,
                                            bool fin = true);
    static std::vector<uint8_t> EncodeText(const std::string& text);
    static std::vector<uint8_t> EncodeBinary(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> EncodeClose(uint16_t statusCode = 1000);
    static std::vector<uint8_t> EncodePong(const std::vector<uint8_t>& payload);

    // Decodes one frame from a byte stream (client frames are unmasked in
    // place). Returns the bytes consumed, 0 when more data is needed, or
    // SIZE_MAX with `error` set on a protocol violation.
    static size_t DecodeFrame(const uint8_t* data, size_t size,
                              Frame& frame, std::string& error);
};

} // namespace Server
} // namespace UltraWeb
