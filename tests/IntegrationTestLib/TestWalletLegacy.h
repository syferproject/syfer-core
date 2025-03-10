// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "CryptoNoteCore/Currency.h"
#include "INode.h"
#include "IWalletLegacy.h"
#include "System/Dispatcher.h"
#include "System/Event.h"
#include "WalletLegacy/WalletLegacy.h"
#include <Logging/ConsoleLogger.h>

namespace Tests
{
namespace common
{

class TestWalletLegacy : private cn::IWalletLegacyObserver
{
public:
  TestWalletLegacy(platform_system::Dispatcher &dispatcher, const cn::Currency &currency, cn::INode &node);
  ~TestWalletLegacy();

  std::error_code init();
  std::error_code sendTransaction(const std::string &address, uint64_t amount, crypto::Hash &txHash);
  void waitForSynchronizationToHeight(uint32_t height);
  cn::IWalletLegacy *wallet();
  cn::AccountPublicAddress address() const;

protected:
  virtual void synchronizationCompleted(std::error_code result) override;
  virtual void synchronizationProgressUpdated(uint32_t current, uint32_t total) override;

private:
  platform_system::Dispatcher &m_dispatcher;
  platform_system::Event m_synchronizationCompleted;
  platform_system::Event m_someTransactionUpdated;

  cn::INode &m_node;
  const cn::Currency &m_currency;
  logging::ConsoleLogger m_logger;
  std::unique_ptr<cn::IWalletLegacy> m_wallet;
  std::unique_ptr<cn::IWalletLegacyObserver> m_walletObserver;
  uint32_t m_currentHeight;
  uint32_t m_synchronizedHeight;
  std::error_code m_lastSynchronizationResult;
};

} // namespace common
} // namespace Tests
