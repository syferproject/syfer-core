// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include "Common/ObserverManager.h"
#include "INode.h"

namespace platform_system {
  class ContextGroup;
  class Dispatcher;
  class Event;
}

namespace cn {

class HttpClient;

class INodeRpcProxyObserver {
public:
  virtual ~INodeRpcProxyObserver() {}
  virtual void connectionStatusUpdated(bool connected) {}
};

class NodeRpcProxy : public cn::INode {
public:
  NodeRpcProxy(const std::string& nodeHost, unsigned short nodePort);
  virtual ~NodeRpcProxy();

  virtual bool addObserver(cn::INodeObserver* observer) override;
  virtual bool removeObserver(cn::INodeObserver* observer) override;

  virtual bool addObserver(cn::INodeRpcProxyObserver* observer);
  virtual bool removeObserver(cn::INodeRpcProxyObserver* observer);

  virtual void init(const Callback& callback) override;
  virtual bool shutdown() override;

  virtual size_t getPeerCount() const override;
  virtual uint32_t getLastLocalBlockHeight() const override;
  virtual uint32_t getLastKnownBlockHeight() const override;
  virtual uint32_t getLocalBlockCount() const override;
  virtual uint32_t getKnownBlockCount() const override;
  virtual uint64_t getLastLocalBlockTimestamp() const override;

  virtual void relayTransaction(const cn::Transaction& transaction, const Callback& callback) override;
  virtual void getRandomOutsByAmounts(std::vector<uint64_t>&& amounts, uint64_t outsCount, std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& result, const Callback& callback) override;
  virtual void getNewBlocks(std::vector<crypto::Hash>&& knownBlockIds, std::vector<cn::block_complete_entry>& newBlocks, uint32_t& startHeight, const Callback& callback) override;
  virtual void getTransactionOutsGlobalIndices(const crypto::Hash& transactionHash, std::vector<uint32_t>& outsGlobalIndices, const Callback& callback) override;
  virtual void queryBlocks(std::vector<crypto::Hash>&& knownBlockIds, uint64_t timestamp, std::vector<BlockShortEntry>& newBlocks, uint32_t& startHeight, const Callback& callback) override;
  virtual void getPoolSymmetricDifference(std::vector<crypto::Hash>&& knownPoolTxIds, crypto::Hash knownBlockId, bool& isBcActual,
          std::vector<std::unique_ptr<ITransactionReader>>& newTxs, std::vector<crypto::Hash>& deletedTxIds, const Callback& callback) override;
  virtual void getMultisignatureOutputByGlobalIndex(uint64_t amount, uint32_t gindex, MultisignatureOutput& out, const Callback& callback) override;
  virtual void getBlocks(const std::vector<uint32_t>& blockHeights, std::vector<std::vector<BlockDetails>>& blocks, const Callback& callback) override;
  
  virtual void getBlocks(const std::vector<crypto::Hash>& blockHashes, std::vector<BlockDetails>& blocks, const Callback& callback) override;
  virtual void getBlocks(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t blocksNumberLimit, std::vector<BlockDetails>& blocks, uint32_t& blocksNumberWithinTimestamps, const Callback& callback) override;
  virtual void getTransactions(const std::vector<crypto::Hash>& transactionHashes, std::vector<TransactionDetails>& transactions, const Callback& callback) override;
  virtual void getTransactionsByPaymentId(const crypto::Hash& paymentId, std::vector<TransactionDetails>& transactions, const Callback& callback) override;
  virtual void getPoolTransactions(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t transactionsNumberLimit, std::vector<TransactionDetails>& transactions, uint64_t& transactionsNumberWithinTimestamps, const Callback& callback) override;
  virtual void isSynchronized(bool& syncStatus, const Callback& callback) override;

  unsigned int rpcTimeout() const { return m_rpcTimeout; }
  void rpcTimeout(unsigned int val) { m_rpcTimeout = val; }

private:
  void resetInternalState();
  void workerThread(const Callback& initialized_callback);

  std::vector<crypto::Hash> getKnownTxsVector() const;
  void pullNodeStatusAndScheduleTheNext();
  void updateNodeStatus();
  void updateBlockchainStatus();
  bool updatePoolStatus();
  void updatePeerCount(size_t peerCount);
  void updatePoolState(const std::vector<std::unique_ptr<ITransactionReader>>& addedTxs, const std::vector<crypto::Hash>& deletedTxsIds);

  std::error_code doRelayTransaction(const cn::Transaction& transaction);
  std::error_code doGetRandomOutsByAmounts(std::vector<uint64_t>& amounts, uint64_t outsCount,
                                           std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& result);

  std::error_code doGetNewBlocks(std::vector<crypto::Hash>& knownBlockIds,
    std::vector<cn::block_complete_entry>& newBlocks, uint32_t& startHeight);
  std::error_code doGetTransactionOutsGlobalIndices(const crypto::Hash& transactionHash,
                                                    std::vector<uint32_t>& outsGlobalIndices);
  std::error_code doQueryBlocksLite(const std::vector<crypto::Hash>& knownBlockIds, uint64_t timestamp,
    std::vector<cn::BlockShortEntry>& newBlocks, uint32_t& startHeight);
  std::error_code doGetPoolSymmetricDifference(std::vector<crypto::Hash>&& knownPoolTxIds, crypto::Hash knownBlockId, bool& isBcActual,
          std::vector<std::unique_ptr<ITransactionReader>>& newTxs, std::vector<crypto::Hash>& deletedTxIds);
  virtual void getTransaction(const crypto::Hash &transactionHash, cn::Transaction &transaction, const Callback &callback) override;
  std::error_code doGetTransaction(const crypto::Hash &transactionHash, cn::Transaction &transaction);

  void scheduleRequest(std::function<std::error_code()>&& procedure, const Callback& callback);
template <typename Request, typename Response>
  std::error_code binaryCommand(const std::string& url, const Request& req, Response& res);
  template <typename Request, typename Response>
  std::error_code jsonCommand(const std::string& url, const Request& req, Response& res);
  template <typename Request, typename Response>
  std::error_code jsonRpcCommand(const std::string& method, const Request& req, Response& res);

  enum State {
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZING,
    STATE_INITIALIZED
  };

private:
  State m_state = STATE_NOT_INITIALIZED;
  std::mutex m_mutex;
  std::condition_variable m_cv_initialized;
  std::thread m_workerThread;
  platform_system::Dispatcher* m_dispatcher = nullptr;
  platform_system::ContextGroup* m_context_group = nullptr;
  tools::ObserverManager<cn::INodeObserver> m_observerManager;
  tools::ObserverManager<cn::INodeRpcProxyObserver> m_rpcProxyObserverManager;

  const std::string m_nodeHost;
  const unsigned short m_nodePort;
  unsigned int m_rpcTimeout;
  HttpClient* m_httpClient = nullptr;
  platform_system::Event* m_httpEvent = nullptr;

  uint64_t m_pullInterval;

  // Internal state
  bool m_stop = false;
  std::atomic<size_t> m_peerCount;
  std::atomic<uint32_t> m_nodeHeight;
  std::atomic<uint32_t> m_networkHeight;

  //protect it with mutex if decided to add worker threads
  crypto::Hash m_lastKnowHash;
  std::atomic<uint64_t> m_lastLocalBlockTimestamp;
  std::unordered_set<crypto::Hash> m_knownTxs;

  bool m_connected;
};

}
