// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>

namespace platform_system {

class Dispatcher;
class Ipv4Address;
class TcpConnection;

class TcpListener {
public:
  TcpListener();
  TcpListener(Dispatcher& dispatcher, const Ipv4Address& address, uint16_t port);
  TcpListener(const TcpListener&) = delete;
  TcpListener(TcpListener&& other);
  ~TcpListener();
  TcpListener& operator=(const TcpListener&) = delete;
  TcpListener& operator=(TcpListener&& other);
  TcpConnection accept();

private:
  Dispatcher* dispatcher;
  void* context;
  int listener;
};

}
