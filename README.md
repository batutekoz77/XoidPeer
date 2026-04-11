# 🚀 XoidPeer

<div align="center">

### A modern C++20 networking layer built on ENet — production-ready, secure, and game-server optimized

</div>

---

## ⚡ What is XoidPeer?

**XoidPeer** is a high-performance C++20 networking library that transforms ENet into a modern, production-ready networking framework.

It removes boilerplate, hides low-level ENet complexity, and adds essential features required for real-world multiplayer systems such as:

- Rate limiting per peer
- Packet validation & security layer
- Bandwidth control presets
- Clean, event-driven API
- Fully encapsulated ENet backend

> Built for game servers, real-time applications, and custom multiplayer systems.

---

## 🎯 Why XoidPeer?

ENet is fast — but low-level and unsafe for production use.

XoidPeer solves that by adding:

- ❌ No raw ENet exposure
- ❌ No manual packet validation
- ❌ No rate control
- ❌ No safety layer

### ✔ XoidPeer gives you:

- 🚀 Clean modern C++20 API
- 🛡 Packet Guard (CRC32 + magic validation)
- ⛔ Per-peer rate limiting (token bucket)
- 📦 Bandwidth management presets
- 🔌 Fully encapsulated networking backend

---

## ⚙️ Features

| Feature | Description |
|--------|-------------|
| 🧠 Clean API | Zero ENet exposure, modern C++20 design |
| 🛡 Packet Guard | CRC32 + magic byte validation |
| ⛔ Rate Limiter | Per-peer token bucket system |
| 📦 Bandwidth Control | Gaming / VoIP / FileTransfer presets |
| 🔁 Event-driven | Lambda-based callbacks |
| 🧵 Thread-safe | Dedicated poll thread + safe state handling |
| 🎮 Game-ready | Designed for real-time multiplayer systems |

---

## 📦 Quick Example

### 🖥 Server

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ServerConfig config;
    config.port = 7777;
    config.maxClients = 64;
    config.channelCount = 2;

    config.rateLimit.enabled = true;
    config.rateLimit.maxPacketsPerSecond = 100;

    config.packetGuard.enabled = true;
    config.checksumEnabled = true;

    XoidPeer::Server server(config);

    server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) {
        printf("[+] Connected: %s:%d
", peer.ip.c_str(), peer.port);
    });

    server.SetOnPacketReceived([&](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
        printf("[Packet] %s
", pkt.AsString().c_str());

        XoidPeer::Packet reply("hello client", XoidPeer::PacketFlag::Reliable);
        server.Send(peer.id, reply);
    });

    server.Start();

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

---

### 📡 Client

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ClientConfig config;
    config.host = "127.0.0.1";
    config.port = 7777;

    XoidPeer::Client client(config);

    client.SetOnConnect([]() {
        printf("[+] Connected!
");
    });

    client.SetOnPacketReceived([](XoidPeer::IncomingPacket pkt) {
        printf("[Server] %s
", pkt.AsString().c_str());
    });

    client.Connect();

    XoidPeer::Packet pkt("hello server", XoidPeer::PacketFlag::Reliable);
    client.Send(pkt);

    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

---

## 🧱 Architecture

```
XoidPeer
├── Public API (include/XoidPeer/)
│   ├── Server
│   ├── Client
│   ├── Packet
│   └── Config
│
├── Core System (src/)
│   ├── Poll Thread
│   ├── Event System
│   ├── Packet Layer
│
├── Security Layer
│   ├── Packet Guard (CRC32 + magic bytes)
│   ├── Rate Limiter (token bucket)
│
└── Vendor
    └── ENet (fully hidden)
```

---

## 🛡 Security Features

- CRC32 packet validation
- Magic byte verification
- Per-peer rate limiting
- Packet drop before callback execution

---

## ⚡ Thread Model

- Dedicated ENet polling thread
- Lock-protected peer state
- Atomic runtime flags
- Minimal contention design

---

## 📜 License

MIT License

---

<div align="center">
Made by <strong>Xoid</strong>
</div>
# 🚀 XoidPeer

<div align="center">

### A modern C++20 networking layer built on ENet — production-ready, secure, and game-server optimized

</div>

---

## ⚡ What is XoidPeer?

**XoidPeer** is a high-performance C++20 networking library that transforms ENet into a modern, production-ready networking framework.

It removes boilerplate, hides low-level ENet complexity, and adds essential features required for real-world multiplayer systems such as:

- Rate limiting per peer
- Packet validation & security layer
- Bandwidth control presets
- Clean, event-driven API
- Fully encapsulated ENet backend

> Built for game servers, real-time applications, and custom multiplayer systems.

---

## 🎯 Why XoidPeer?

ENet is fast — but low-level and unsafe for production use.

XoidPeer solves that by adding:

- ❌ No raw ENet exposure
- ❌ No manual packet validation
- ❌ No rate control
- ❌ No safety layer

### ✔ XoidPeer gives you:

- 🚀 Clean modern C++20 API
- 🛡 Packet Guard (CRC32 + magic validation)
- ⛔ Per-peer rate limiting (token bucket)
- 📦 Bandwidth management presets
- 🔌 Fully encapsulated networking backend

---

## ⚙️ Features

| Feature | Description |
|--------|-------------|
| 🧠 Clean API | Zero ENet exposure, modern C++20 design |
| 🛡 Packet Guard | CRC32 + magic byte validation |
| ⛔ Rate Limiter | Per-peer token bucket system |
| 📦 Bandwidth Control | Gaming / VoIP / FileTransfer presets |
| 🔁 Event-driven | Lambda-based callbacks |
| 🧵 Thread-safe | Dedicated poll thread + safe state handling |
| 🎮 Game-ready | Designed for real-time multiplayer systems |

---

## 📦 Quick Example

### 🖥 Server

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ServerConfig config;
    config.port = 7777;
    config.maxClients = 64;
    config.channelCount = 2;

    config.rateLimit.enabled = true;
    config.rateLimit.maxPacketsPerSecond = 100;

    config.packetGuard.enabled = true;
    config.checksumEnabled = true;

    XoidPeer::Server server(config);

    server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) {
        printf("[+] Connected: %s:%d
", peer.ip.c_str(), peer.port);
    });

    server.SetOnPacketReceived([&](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
        printf("[Packet] %s
", pkt.AsString().c_str());

        XoidPeer::Packet reply("hello client", XoidPeer::PacketFlag::Reliable);
        server.Send(peer.id, reply);
    });

    server.Start();

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

---

### 📡 Client

```cpp
#include <XoidPeer/XoidPeer.h>

int main()
{
    XoidPeer::ClientConfig config;
    config.host = "127.0.0.1";
    config.port = 7777;

    XoidPeer::Client client(config);

    client.SetOnConnect([]() {
        printf("[+] Connected!
");
    });

    client.SetOnPacketReceived([](XoidPeer::IncomingPacket pkt) {
        printf("[Server] %s
", pkt.AsString().c_str());
    });

    client.Connect();

    XoidPeer::Packet pkt("hello server", XoidPeer::PacketFlag::Reliable);
    client.Send(pkt);

    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

---

## 🧱 Architecture

```
XoidPeer
├── Public API (include/XoidPeer/)
│   ├── Server
│   ├── Client
│   ├── Packet
│   └── Config
│
├── Core System (src/)
│   ├── Poll Thread
│   ├── Event System
│   ├── Packet Layer
│
├── Security Layer
│   ├── Packet Guard (CRC32 + magic bytes)
│   ├── Rate Limiter (token bucket)
│
└── Vendor
    └── ENet (fully hidden)
```

---

## 🛡 Security Features

- CRC32 packet validation
- Magic byte verification
- Per-peer rate limiting
- Packet drop before callback execution

---

## ⚡ Thread Model

- Dedicated ENet polling thread
- Lock-protected peer state
- Atomic runtime flags
- Minimal contention design

---

## 📜 License

MIT License

---

<div align="center">
Made by <strong>Xoid</strong>
</div>
