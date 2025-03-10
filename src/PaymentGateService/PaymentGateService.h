// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "ConfigurationManager.h"
#include "PaymentServiceConfiguration.h"

#include "Logging/ConsoleLogger.h"
#include "Logging/LoggerGroup.h"
#include "Logging/StreamLogger.h"

#include "PaymentGate/NodeFactory.h"
#include "PaymentGate/WalletService.h"

class PaymentGateService {
public:
  PaymentGateService() : currencyBuilder(logger)
  {
  }

  bool init(int argc, char** argv);

  const payment_service::ConfigurationManager& getConfig() const { return config; }
  payment_service::WalletConfiguration getWalletConfig() const;
  const cn::Currency getCurrency();

  void run();
  void stop();
  
  logging::ILogger& getLogger() { return logger; }

private:

  void runInProcess(const logging::LoggerRef& log);
  void runRpcProxy(const logging::LoggerRef& log);

  void runWalletService(const cn::Currency& currency, cn::INode& node);

  platform_system::Dispatcher* dispatcher = nullptr;
  platform_system::Event* stopEvent = nullptr;
  payment_service::ConfigurationManager config;
  payment_service::WalletService* service = nullptr;
  cn::CurrencyBuilder currencyBuilder;
  
  logging::LoggerGroup logger;
  std::ofstream fileStream;
  logging::StreamLogger fileLogger;
  logging::ConsoleLogger consoleLogger;
};
