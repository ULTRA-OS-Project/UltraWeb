// UltraWeb/server/WebSocketHandler.cpp
// RFC 6455 WebSocket protocol implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "WebSocketHandler.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace UltraWeb {
namespace Server {

// ============================================================================
// SHA-1 (FIPS 180-1) - handshake only, not used for security-critical work
// ============================================================================

std::vector<uint8_t> SHA1(const uint8_t* data, size_t size) {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    // Padded message: data + 0x80 + zeros + 64-bit big-endian bit length
    uint64_t bitLen = static_cast<uint64_t>(size) * 8;
    size_t paddedLen = ((size + 8) / 64 + 1) * 64;
    std::vector<uint8_t> msg(paddedLen, 0);
    std::memcpy(msg.data(), data, size);
    msg[size] = 0x80;
    for (int i = 0; i < 8; i++) {
        msg[paddedLen - 1 - i] = static_cast<uint8_t>(bitLen >> (8 * i));
    }

    auto rol = [](uint32_t v, int bits) {
        return (v << bits) | (v >> (32 - bits));
    };

    for (size_t chunk = 0; chunk < paddedLen; chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(msg[chunk + i * 4 + 3]);
        }
        for (int i = 16; i < 80; i++) {
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d);           k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                    k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d);  k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                    k = 0xCA62C1D6; }

            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }

    std::vector<uint8_t> digest(20);
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            digest[i * 4 + j] = static_cast<uint8_t>(h[i] >> (24 - 8 * j));
        }
    }
    return digest;
}

// ============================================================================
// BASE64
// ============================================================================

std::string Base64Encode(const uint8_t* data, size_t size) {
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((size + 2) / 3) * 4);

    for (size_t i = 0; i < size; i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < size) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < size) n |= data[i + 2];

        out += alphabet[(n >> 18) & 63];
        out += alphabet[(n >> 12) & 63];
        out += (i + 1 < size) ? alphabet[(n >> 6) & 63] : '=';
        out += (i + 2 < size) ? alphabet[n & 63] : '=';
    }
    return out;
}

// ============================================================================
// HANDSHAKE
// ============================================================================

std::string WebSocketHandler::ComputeAcceptKey(const std::string& clientKey) {
    static const char* kGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = clientKey + kGuid;
    std::vector<uint8_t> digest =
        SHA1(reinterpret_cast<const uint8_t*>(combined.data()), combined.size());
    return Base64Encode(digest.data(), digest.size());
}

bool WebSocketHandler::ParseUpgradeRequest(const std::string& httpRequest,
                                           std::string& clientKey) {
    clientKey.clear();

    // Case-insensitive header scan
    auto findHeader = [&](const std::string& name) -> std::string {
        std::string lower;
        lower.reserve(httpRequest.size());
        for (char c : httpRequest) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        std::string needle = "\n" + name + ":";
        size_t pos = lower.find(needle);
        if (pos == std::string::npos) return "";
        size_t start = pos + needle.size();
        size_t end = httpRequest.find("\r\n", start);
        if (end == std::string::npos) end = httpRequest.find('\n', start);
        if (end == std::string::npos) end = httpRequest.size();
        std::string value = httpRequest.substr(start, end - start);
        // Trim
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
        return value;
    };

    std::string upgrade = findHeader("upgrade");
    std::transform(upgrade.begin(), upgrade.end(), upgrade.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (upgrade != "websocket") return false;

    std::string key = findHeader("sec-websocket-key");
    if (key.empty()) return false;

    clientKey = key;
    return true;
}

std::string WebSocketHandler::BuildUpgradeResponse(const std::string& clientKey) {
    return "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: " + ComputeAcceptKey(clientKey) + "\r\n"
           "\r\n";
}

// ============================================================================
// FRAMES
// ============================================================================

std::vector<uint8_t> WebSocketHandler::EncodeFrame(OpCode opcode,
                                                   const uint8_t* payload,
                                                   size_t size, bool fin) {
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>((fin ? 0x80 : 0x00) |
                                       static_cast<uint8_t>(opcode)));
    if (size < 126) {
        out.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        out.push_back(126);
        out.push_back(static_cast<uint8_t>(size >> 8));
        out.push_back(static_cast<uint8_t>(size & 0xFF));
    } else {
        out.push_back(127);
        for (int i = 7; i >= 0; i--) {
            out.push_back(static_cast<uint8_t>(
                (static_cast<uint64_t>(size) >> (8 * i)) & 0xFF));
        }
    }
    if (payload && size) out.insert(out.end(), payload, payload + size);
    return out;
}

std::vector<uint8_t> WebSocketHandler::EncodeText(const std::string& text) {
    return EncodeFrame(OpCode::Text,
                       reinterpret_cast<const uint8_t*>(text.data()),
                       text.size());
}

std::vector<uint8_t> WebSocketHandler::EncodeBinary(
    const std::vector<uint8_t>& data) {
    return EncodeFrame(OpCode::Binary, data.data(), data.size());
}

std::vector<uint8_t> WebSocketHandler::EncodeClose(uint16_t statusCode) {
    uint8_t payload[2] = {static_cast<uint8_t>(statusCode >> 8),
                          static_cast<uint8_t>(statusCode & 0xFF)};
    return EncodeFrame(OpCode::Close, payload, 2);
}

std::vector<uint8_t> WebSocketHandler::EncodePong(
    const std::vector<uint8_t>& payload) {
    return EncodeFrame(OpCode::Pong, payload.data(), payload.size());
}

size_t WebSocketHandler::DecodeFrame(const uint8_t* data, size_t size,
                                     Frame& frame, std::string& error) {
    error.clear();
    if (size < 2) return 0;

    frame.fin = (data[0] & 0x80) != 0;
    uint8_t rsv = data[0] & 0x70;
    if (rsv != 0) {
        error = "RSV bits set without negotiated extension";
        return SIZE_MAX;
    }
    frame.opcode = static_cast<OpCode>(data[0] & 0x0F);

    bool masked = (data[1] & 0x80) != 0;
    uint64_t payloadLen = data[1] & 0x7F;
    size_t pos = 2;

    if (payloadLen == 126) {
        if (size < pos + 2) return 0;
        payloadLen = (static_cast<uint64_t>(data[pos]) << 8) | data[pos + 1];
        pos += 2;
    } else if (payloadLen == 127) {
        if (size < pos + 8) return 0;
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | data[pos + i];
        }
        pos += 8;
    }

    uint8_t maskKey[4] = {0, 0, 0, 0};
    if (masked) {
        if (size < pos + 4) return 0;
        std::memcpy(maskKey, data + pos, 4);
        pos += 4;
    }

    if (payloadLen > (1ULL << 30)) {
        error = "frame payload exceeds 1GB limit";
        return SIZE_MAX;
    }
    if (size < pos + payloadLen) return 0;

    frame.payload.assign(data + pos, data + pos + payloadLen);
    if (masked) {
        for (size_t i = 0; i < frame.payload.size(); i++) {
            frame.payload[i] ^= maskKey[i % 4];
        }
    }
    return pos + static_cast<size_t>(payloadLen);
}

} // namespace Server
} // namespace UltraWeb
