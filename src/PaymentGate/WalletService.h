// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <System/ContextGroup.h>
#include <System/Dispatcher.h>
#include <System/Event.h>
#include "IWallet.h"
#include "INode.h"
#include "CryptoNoteCore/Currency.h"
#include "PaymentServiceJsonRpcMessages.h"
#undef ERROR //TODO: workaround for windows build. fix it
#include "Logging/LoggerRef.h"

#include <fstream>
#include <memory>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace cn
{
class IFusionManager;
}

namespace payment_service
{

struct WalletConfiguration
{
  std::string walletFile;
  std::string walletPassword;
  std::string secretSpendKey;
  std::string secretViewKey;
};

void generateNewWallet(const cn::Currency &currency, const WalletConfiguration &conf, logging::ILogger &logger, platform_system::Dispatcher &dispatcher);

struct TransactionsInBlockInfoFilter;

class WalletService
{
public:
  WalletService(const cn::Currency &currency, platform_system::Dispatcher &sys, cn::INode &node, cn::IWallet &wallet, cn::IFusionManager &fusionManager, const WalletConfiguration &conf, logging::ILogger &logger, bool testnet);
  virtual ~WalletService();

  void init();
  void saveWallet();

  std::error_code saveWalletNoThrow();
  std::error_code resetWallet();
  std::error_code resetWallet(const uint32_t scanHeight);
  std::error_code exportWallet(const std::string &fileName, bool keysOnly = false);
  std::error_code replaceWithNewWallet(const std::string &viewSecretKey);
  std::error_code createAddress(const std::string &spendSecretKeyText, std::string &address);
  std::error_code createAddress(std::string &address);
  std::error_code createAddressList(const std::vector<std::string> &spendSecretKeysText, bool reset, std::vector<std::string> &addresses);
  std::error_code createTrackingAddress(const std::string &spendPublicKeyText, std::string &address);
  std::error_code deleteAddress(const std::string &address);
  std::error_code getSpendkeys(const std::string &address, std::string &publicSpendKeyText, std::string &secretSpendKeyText);
  std::error_code getBalance(const std::string &address, uint64_t &availableBalance, uint64_t &lockedAmount, uint64_t &lockedDepositBalance, uint64_t &unlockedDepositBalance);
  std::error_code getBalance(uint64_t &availableBalance, uint64_t &lockedAmount, uint64_t &lockedDepositBalance, uint64_t &unlockedDepositBalance);
  std::error_code getBlockHashes(uint32_t firstBlockIndex, uint32_t blockCount, std::vector<std::string> &blockHashes);
std::error_code getViewKey(std::string &viewSecretKey);
  std::error_code getTransactionHashes(const std::vector<std::string> &addresses, const std::string &blockHash,
                                       uint32_t blockCount, const std::string &paymentId, std::vector<TransactionHashesInBlockRpcInfo> &transactionHashes);
  std::error_code getTransactionHashes(const std::vector<std::string> &addresses, uint32_t firstBlockIndex,
                                       uint32_t blockCount, const std::string &paymentId, std::vector<TransactionHashesInBlockRpcInfo> &transactionHashes);
  std::error_code getTransactions(const std::vector<std::string> &addresses, const std::string &blockHash,
                                  uint32_t blockCount, const std::string &paymentId, std::vector<TransactionsInBlockRpcInfo> &transactionHashes);
  std::error_code getTransactions(const std::vector<std::string> &addresses, uint32_t firstBlockIndex,
                                  uint32_t blockCount, const std::string &paymentId, std::vector<TransactionsInBlockRpcInfo> &transactionHashes);
  std::error_code getTransaction(const std::string &transactionHash, TransactionRpcInfo &transaction);
  std::error_code getAddresses(std::vector<std::string> &addresses);
  std::error_code sendTransaction(const SendTransaction::Request &request, std::string &transactionHash, std::string &transactionSecretKey);
  std::error_code createDelayedTransaction(const CreateDelayedTransaction::Request &request, std::string &transactionHash);
  std::error_code createIntegratedAddress(const CreateIntegrated::Request &request, std::string &integrated_address);
  std::error_code splitIntegratedAddress(const SplitIntegrated::Request &request, std::string &address, std::string &payment_id);
  std::error_code getDelayedTransactionHashes(std::vector<std::string> &transactionHashes);
  std::error_code deleteDelayedTransaction(const std::string &transactionHash);
  std::error_code sendDelayedTransaction(const std::string &transactionHash);
  std::error_code getUnconfirmedTransactionHashes(const std::vector<std::string> &addresses, std::vector<std::string> &transactionHashes);
  std::error_code getStatus(uint32_t &blockCount, uint32_t &knownBlockCount, std::string &lastBlockHash, uint32_t &peerCount, uint32_t &depositCount, uint32_t &transactionCount, uint32_t &addressCount);
  std::error_code createDeposit(uint64_t amount, uint64_t term, std::string sourceAddress, std::string &transactionHash);
  std::error_code withdrawDeposit(uint64_t depositId, std::string &transactionHash);
  std::error_code sendDeposit(uint64_t amount, uint64_t term, std::string sourceAddress, std::string destinationAddress, std::string &transactionHash);
  std::error_code getDeposit(uint64_t depositId, uint64_t &amount, uint64_t &term, uint64_t &interest, std::string &creatingTransactionHash, std::string &spendingTransactionHash, bool &locked, uint64_t &height, uint64_t &unlockHeight, std::string &address);

  std::error_code getMessagesFromExtra(const std::string &extra, std::vector<std::string> &messges);
  std::error_code estimateFusion(uint64_t threshold, const std::vector<std::string> &addresses, uint32_t &fusionReadyCount, uint32_t &totalOutputCount);
  std::error_code sendFusionTransaction(uint64_t threshold, uint32_t anonymity, const std::vector<std::string> &addresses,
                                        const std::string &destinationAddress, std::string &transactionHash);

private:
  void refresh();
  void reset();

  void loadWallet();
  void loadTransactionIdIndex();

  void replaceWithNewWallet(const crypto::SecretKey &viewSecretKey);

  std::vector<cn::TransactionsInBlockInfo> getTransactions(const crypto::Hash &blockHash, size_t blockCount) const;
  std::vector<cn::TransactionsInBlockInfo> getTransactions(uint32_t firstBlockIndex, size_t blockCount) const;

  std::vector<cn::DepositsInBlockInfo> getDeposits(const crypto::Hash &blockHash, size_t blockCount) const;
  std::vector<cn::DepositsInBlockInfo> getDeposits(uint32_t firstBlockIndex, size_t blockCount) const;

  std::vector<TransactionHashesInBlockRpcInfo> getRpcTransactionHashes(const crypto::Hash &blockHash, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const;
  std::vector<TransactionHashesInBlockRpcInfo> getRpcTransactionHashes(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const;

  std::vector<TransactionsInBlockRpcInfo> getRpcTransactions(const crypto::Hash &blockHash, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const;
  std::vector<TransactionsInBlockRpcInfo> getRpcTransactions(uint32_t firstBlockIndex, size_t blockCount, const TransactionsInBlockInfoFilter &filter) const;

  TransactionRpcInfo convertTransactionWithTransfersToTransactionRpcInfo(
      const cn::WalletTransactionWithTransfers &transactionWithTransfers, const uint32_t &knownBlockCount) const;
  std::vector<TransactionsInBlockRpcInfo> convertTransactionsInBlockInfoToTransactionsInBlockRpcInfo(
      const std::vector<cn::TransactionsInBlockInfo> &blocks, const uint32_t &knownBlockCount) const;

  const cn::Currency &currency;
  cn::IWallet &wallet;
  cn::IFusionManager &fusionManager;
  cn::INode &node;
  const WalletConfiguration &config;
  bool inited;
  logging::LoggerRef logger;
  platform_system::Dispatcher &dispatcher;
  platform_system::Event readyEvent;
  platform_system::ContextGroup refreshContext;
  std::map<std::string, size_t> transactionIdIndex;
  bool m_testnet;
};

} //namespace payment_service
