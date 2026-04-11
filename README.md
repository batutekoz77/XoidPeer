<div align="center">

# XoidPeer

**A modern C++20 networking library built on top of ENet**  
Rate limiting · Packet protection · Clean server/client API · Growtanya protocol support

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)
![Version](https://img.shields.io/badge/Version-1.0.0-orange?style=flat-square)

</div>

---

## Overview

XoidPeer is a high-performance C++ networking library that wraps [ENet](http://enet.bespin.org/) with a clean, modern API. It eliminates boilerplate, enforces best practices, and adds production-ready features like rate limiting, CRC32 packet validation, and bandwidth management — all without exposing a single ENet symbol to the user.

```cpp
#include <XoidPeer/XoidPeer.h>

XoidPeer::ServerConfig config;
config.port                    = 7777;
config.maxClients              = 64;
config.usingNewPacketForServer = true;   // Growtopia protocol
config.checksumEnabled         = true;
config.compressEnabled         = true;
config.rateLimit.maxPacketsPerSecond = 100;

XoidPeer::Server server(config);

server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) {
    printf("connected: %s:%d\n", peer.ip.c_str(), peer.port);
});

server.SetOnPacketReceived([](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
    printf("packet from peer %u: %s\n", peer.id, pkt.AsString().c_str());
});

server.Start();
```

---

## Features

| Feature | Description |
|---|---|
| **Clean API** | No ENet types exposed. Zero include pollution. |
| **Rate Limiter** | Per-peer token bucket algorithm — controls packets/sec and bytes/sec independently |
| **Packet Guard** | Magic header validation + CRC32 checksum — malformed packets dropped before callbacks |
| **Bandwidth Control** | Presets (Gaming, VoIP, FileTransfer) or fully custom Kbps configuration |
| **Growtopia Protocol** | Built-in `usingNewPacket` / `usingNewPacketForServer` support for GT private servers |
| **CRC32 + Compression** | Optional `enet_crc32` checksum and range coder compression per host |
| **Thread Safety** | Poll loop runs on a dedicated thread, all shared state protected with `std::scoped_lock` |
| **Modern C++20** | `std::span`, `std::optional`, concepts, `consteval` CRC table, `[[nodiscard]]` everywhere |

---

## Architecture

```
XoidPeer/
├── include/
│   └── XoidPeer/
│       ├── XoidPeer.h          ← single public header
│       ├── Config.h            ← ServerConfig, ClientConfig, presets
│       ├── Server.h            ← Server class
│       ├── Client.h            ← Client class
│       └── Packet.h            ← Packet, IncomingPacket
├── src/
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Packet.cpp
│   ├── RateLimiter.h/.cpp      ← internal, not exposed
│   └── PacketGuard.h/.cpp      ← internal, not exposed
└── vendor/
    └── enet/                   ← fully encapsulated
```

Users only ever see `include/XoidPeer/`. ENet is completely hidden inside the library.

---

## Installation

### Requirements
- Visual Studio 2022+ (C++20)
- Windows x64

### Add to your project

1. Copy `include/` folder into your project
2. Link `XoidPeer.lib` in **Linker → Input → Additional Dependencies**
3. Add `include/` to **C/C++ → General → Additional Include Directories**
4. Done — no ENet setup needed

```cpp
#include <XoidPeer/XoidPeer.h>
```

---

## Usage

### Server

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ServerConfig config;
    config.port                          = 7777;
    config.maxClients                    = 64;
    config.channelCount                  = 2;
    config.checksumEnabled               = true;
    config.compressEnabled               = true;
    config.usingNewPacketForServer       = true;   // GT protocol
    config.rateLimit.enabled             = true;
    config.rateLimit.maxPacketsPerSecond = 100;
    config.rateLimit.maxBytesPerSecond   = 1024 * 1024;
    config.rateLimit.burstSize           = 20;
    config.packetGuard.enabled           = true;
    config.packetGuard.checksumEnabled   = true;
    config.packetGuard.magicBytesEnabled = true;
    config.bandwidth.preset              = XoidPeer::BandwidthPreset::Gaming;

    XoidPeer::Server server(config);

    server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) {
        printf("[+] %s:%d connected (id: %u)\n", peer.ip.c_str(), peer.port, peer.id);
    });

    server.SetOnClientDisconnect([](const XoidPeer::PeerInfo& peer) {
        printf("[-] peer %u disconnected\n", peer.id);
    });

    server.SetOnPacketReceived([&server](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
        printf("[pkt] peer %u | channel %u | %zu bytes\n", peer.id, pkt.channel, pkt.data.size());

        // echo back
        XoidPeer::Packet reply(std::span<const uint8_t>(pkt.data), XoidPeer::PacketFlag::Reliable);
        server.Send(peer.id, reply);
    });

    if (!server.Start()) {
        printf("[ERR] Failed to start server\n");
        return 1;
    }

    printf("[*] Server listening on port %d\n", config.port);

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

---

### Client

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ClientConfig config;
    config.host               = "127.0.0.1";
    config.port               = 7777;
    config.channelCount       = 2;
    config.timeoutMs          = 5000;
    config.checksumEnabled    = true;
    config.compressEnabled    = true;
    config.usingNewPacket     = true;   // GT protocol

    XoidPeer::Client client(config);

    client.SetOnConnect([]() {
        printf("[+] Connected to server\n");
    });

    client.SetOnDisconnect([]() {
        printf("[-] Disconnected from server\n");
    });

    client.SetOnConnectFailed([]() {
        printf("[!] Connection timed out\n");
    });

    client.SetOnPacketReceived([](XoidPeer::IncomingPacket pkt) {
        printf("[pkt] channel %u | %zu bytes | %s\n",
            pkt.channel, pkt.data.size(), pkt.AsString().c_str());
    });

    if (!client.Connect()) {
        printf("[ERR] Failed to initiate connection\n");
        return 1;
    }

    // wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // send a text packet
    XoidPeer::Packet pkt("hello from client", XoidPeer::PacketFlag::Reliable);
    client.Send(pkt);

    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

---

### Sending structs directly

```cpp
struct PlayerMove
{
    float x, y;
    uint8_t direction;
};

PlayerMove move{ 120.5f, 340.0f, 2 };
auto pkt = XoidPeer::Packet::FromStruct(move, XoidPeer::PacketFlag::Unreliable, 1);
server.Send(peerId, pkt);

// receiving side
server.SetOnPacketReceived([](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
    auto move = pkt.As<PlayerMove>();
    printf("player moved to %.1f %.1f\n", move.x, move.y);
});
```

---

### Broadcast

```cpp
XoidPeer::Packet announcement("server is restarting", XoidPeer::PacketFlag::Reliable);

// to all
server.Broadcast(announcement);

// to all except one peer
server.BroadcastExcept(senderPeerId, announcement);
```

---

## Configuration Reference

### ServerConfig

| Field | Type | Default | Description |
|---|---|---|---|
| `port` | `uint16_t` | `7777` | Listening port |
| `maxClients` | `uint32_t` | `32` | Maximum simultaneous connections |
| `channelCount` | `uint8_t` | `2` | ENet channel count |
| `usingNewPacketForServer` | `bool` | `false` | GT modified ENet flag |
| `checksumEnabled` | `bool` | `false` | Enable `enet_crc32` |
| `compressEnabled` | `bool` | `false` | Enable range coder compression |
| `bandwidth` | `BandwidthConfig` | Gaming preset | Bandwidth limits |
| `rateLimit` | `RateLimitConfig` | enabled, 100 pps | Per-peer rate limiting |
| `packetGuard` | `PacketGuardConfig` | enabled | Magic + CRC validation |

### ClientConfig

| Field | Type | Default | Description |
|---|---|---|---|
| `host` | `std::string` | `"127.0.0.1"` | Server address |
| `port` | `uint16_t` | `7777` | Server port |
| `channelCount` | `uint8_t` | `2` | ENet channel count |
| `timeoutMs` | `uint32_t` | `5000` | Connection timeout in ms |
| `usingNewPacket` | `bool` | `false` | GT modified ENet flag |
| `checksumEnabled` | `bool` | `false` | Enable `enet_crc32` |
| `compressEnabled` | `bool` | `false` | Enable range coder compression |
| `bandwidth` | `BandwidthConfig` | Gaming preset | Bandwidth limits |
| `packetGuard` | `PacketGuardConfig` | enabled | CRC validation on receive |

### BandwidthPreset

| Preset | Incoming | Outgoing |
|---|---|---|
| `Unlimited` | ∞ | ∞ |
| `Gaming` | 2 Mbps | 2 Mbps |
| `VoIP` | 256 Kbps | 256 Kbps |
| `FileTransfer` | 10 Mbps | 10 Mbps |
| `Custom` | `incomingKbps` | `outgoingKbps` |

---

## How It Works

### Rate Limiter — Token Bucket

Each connected peer gets its own token bucket. Tokens refill at `maxPacketsPerSecond` rate per second and burst up to `burstSize`. A packet is dropped before reaching user callbacks if either the token bucket is empty or the byte budget for the current second is exceeded.

```
tokens += elapsed_seconds * maxPacketsPerSecond
tokens  = min(tokens, burstSize)

on packet arrive:
    if tokens < 1        → DROP
    if bytes > budget    → DROP
    tokens -= 1
    bytes  += packetSize
```

### Packet Guard — CRC32

The CRC32 lookup table is generated at compile time via `consteval`. On receive, XoidPeer validates the magic header (`0x584F4944` = `"XOID"`) and recomputes the CRC32 over the payload. Mismatches are silently dropped.

---

## Thread Model

```
main thread          poll thread
──────────           ──────────────────────────────────────
Server::Start()  →   PollLoop()
                         enet_host_service() [1ms]
                         HandleConnect()
                         HandleDisconnect()  → RateLimiter::RemovePeer()
                         HandlePacket()      → RateLimiter::Check()
                                             → PacketGuard::Validate()
                                             → m_onPacket callback
```

All access to `m_peers` is guarded by `std::scoped_lock`. Atomic variables (`m_running`, `m_state`, `m_ping`, `m_nextPeerId`) use explicit `memory_order` for minimal overhead.

---

## License

MIT License — see [LICENSE](LICENSE)

---

<div align="center">
Made by <strong>Xoid</strong>
</div>