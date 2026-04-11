#include <XoidPeer/XoidPeer.h>
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    XoidPeer::ServerConfig config;
    config.port = 17091;
    config.maxClients = 64;
    config.channelCount = 2;
    config.usingNewPacketForServer = true;
    config.checksumEnabled = true;
    config.compressEnabled = true;
    config.rateLimit.enabled = true;
    config.rateLimit.maxPacketsPerSecond = 100;
    config.rateLimit.maxBytesPerSecond = 1024 * 1024;
    config.rateLimit.burstSize = 20;
    config.packetGuard.enabled = true;
    config.packetGuard.checksumEnabled = true;
    config.packetGuard.magicBytesEnabled = true;
    config.bandwidth.preset = XoidPeer::BandwidthPreset::Gaming;

    XoidPeer::Server server(config);

    server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) {
        std::cout << "[+] connected   | id: " << peer.id
            << " | " << peer.ip << ":" << peer.port << "\n";
        });

    server.SetOnClientDisconnect([](const XoidPeer::PeerInfo& peer) {
        std::cout << "[-] disconnected | id: " << peer.id << "\n";
        });

    server.SetOnPacketReceived([&server](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) {
        std::cout << "[>] peer " << peer.id
            << " | ch " << static_cast<int>(pkt.channel)
            << " | " << pkt.data.size() << " bytes"
            << " | \"" << pkt.AsString() << "\"\n";

        // echo back
        XoidPeer::Packet reply(
            std::span<const uint8_t>(pkt.data),
            XoidPeer::PacketFlag::Reliable
        );
        server.Send(peer.id, reply);
        });

    if (!server.Start())
    {
        std::cerr << "[ERR] failed to start server on port " << config.port << "\n";
        return 1;
    }

    std::cout << "[*] XoidPeer server listening on port " << config.port << "\n";
    std::cout << "[*] press ENTER to stop\n\n";

    std::cin.get();

    server.Stop();
    std::cout << "[*] server stopped\n";

    return 0;
}