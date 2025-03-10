// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "CryptoNoteCore/CoreConfig.h"
#include "PaymentServiceConfiguration.h"
#include "P2p/NetNodeConfig.h"
#include "RpcNodeConfiguration.h"

namespace payment_service {

class ConfigurationManager {
public:
  ConfigurationManager() = default;
  bool init(int argc, char** argv);

  bool startInprocess = false;
  Configuration gateConfiguration;
  cn::NetNodeConfig netNodeConfig;
  cn::CoreConfig coreConfig;
  RpcNodeConfiguration remoteNodeConfig;
};

} //namespace payment_service
