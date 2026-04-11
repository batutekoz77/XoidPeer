#pragma once

// ─────────────────────────────────────────────────────────────────
//  XoidPeer — ENet networking library
//  by Xoid
//
//  Usage:
//      #include <XoidPeer/XoidPeer.h>
//
//  Server:
//      XoidPeer::Server server(config);
//      server.SetOnClientConnect([](const XoidPeer::PeerInfo& peer) { ... });
//      server.SetOnPacketReceived([](const XoidPeer::PeerInfo& peer, XoidPeer::IncomingPacket pkt) { ... });
//      server.Start();
//
//  Client:
//      XoidPeer::Client client(config);
//      client.SetOnConnect([]() { ... });
//      client.SetOnPacketReceived([](XoidPeer::IncomingPacket pkt) { ... });
//      client.Connect();
// ─────────────────────────────────────────────────────────────────

#include "Config.h"
#include "Packet.h"
#include "Server.h"
#include "Client.h"

// ─────────────────────────────────────────
//  Version
// ─────────────────────────────────────────
#define XOIDPEER_VERSION_MAJOR 1
#define XOIDPEER_VERSION_MINOR 0
#define XOIDPEER_VERSION_PATCH 0
#define XOIDPEER_VERSION "1.0.0"