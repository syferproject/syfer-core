// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <boost/utility.hpp>

// multi index
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include "Common/Util.h"
#include "Common/int-util.h"
#include "Common/ObserverManager.h"
#include "crypto/hash.h"

#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/ITimeProvider.h"
#include "CryptoNoteCore/ITransactionValidator.h"
#include "CryptoNoteCore/ITxPoolObserver.h"
#include "CryptoNoteCore/VerificationContext.h"
#include "CryptoNoteCore/BlockchainIndices.h"

#include <Logging/LoggerRef.h>

namespace cn {

  class ISerializer;

  class OnceInTimeInterval {
  public:
    OnceInTimeInterval(unsigned interval, cn::ITimeProvider& timeProvider)
      : m_interval(interval), m_timeProvider(timeProvider) {
      m_lastWorkedTime = 0;
    }

    template<class functor_t>
    bool call(functor_t functr) {
      time_t now = m_timeProvider.now();

      if (now - m_lastWorkedTime > m_interval) {
        bool res = functr();
        m_lastWorkedTime = m_timeProvider.now();
        return res;
      }

      return true;
    }

  private:
    time_t m_lastWorkedTime;
    unsigned m_interval;
    cn::ITimeProvider& m_timeProvider;
  };

  using cn::BlockInfo;
  using namespace boost::multi_index;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class tx_memory_pool: boost::noncopyable {
  public:
    tx_memory_pool(
      const cn::Currency& currency, 
      cn::ITransactionValidator& validator,
      cn::ITimeProvider& timeProvider,
      logging::ILogger& log);

    bool addObserver(ITxPoolObserver* observer);
    bool removeObserver(ITxPoolObserver* observer);

    // load/store operations
    bool init(const std::string& config_folder);
    bool deinit();

    bool have_tx(const crypto::Hash &id) const;
    bool add_tx(const Transaction &tx, const crypto::Hash &id, size_t blobSize, tx_verification_context& tvc, bool keeped_by_block, uint32_t height);
    bool add_tx(const Transaction &tx, tx_verification_context& tvc, bool keeped_by_block, uint32_t height);
    //gets tx and remove it from pool
    bool take_tx(const crypto::Hash &id, Transaction &tx, size_t& blobSize, uint64_t& fee);

    bool on_blockchain_inc(uint64_t new_block_height, const crypto::Hash& top_block_id);
    bool on_blockchain_dec(uint64_t new_block_height, const crypto::Hash& top_block_id);

    void lock() const;
    void unlock() const;
    std::unique_lock<std::recursive_mutex> obtainGuard() const;

    bool fill_block_template(Block &bl, size_t median_size, size_t maxCumulativeSize, uint64_t already_generated_coins, size_t &total_size, uint64_t &fee, uint32_t& height);

    void get_transactions(std::list<Transaction>& txs) const;
    void get_difference(const std::vector<crypto::Hash>& known_tx_ids, std::vector<crypto::Hash>& new_tx_ids, std::vector<crypto::Hash>& deleted_tx_ids) const;
    size_t get_transactions_count() const;
    std::string print_pool(bool short_format) const;
    void on_idle();

    bool getTransactionIdsByPaymentId(const crypto::Hash& paymentId, std::vector<crypto::Hash>& transactionIds);
    bool getTransactionIdsByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t transactionsNumberLimit, std::vector<crypto::Hash>& hashes, uint64_t& transactionsNumberWithinTimestamps);
    bool getTransaction(const crypto::Hash &id, Transaction &tx);
    
    template<class t_ids_container, class t_tx_container, class t_missed_container>
    void getTransactions(const t_ids_container& txsIds, t_tx_container& txs, t_missed_container& missedTxs) {
      std::lock_guard<std::recursive_mutex> lock(m_transactions_lock);

      for (const auto& id : txsIds) {
        auto it = m_transactions.find(id);
        if (it == m_transactions.end()) {
          missedTxs.push_back(id);
        } else {
          txs.push_back(it->tx);
        }
      }
    }

    void serialize(ISerializer& s);

    struct TransactionCheckInfo {
      BlockInfo maxUsedBlock;
      BlockInfo lastFailedBlock;
    };

    struct TransactionDetails : public TransactionCheckInfo {
      crypto::Hash id;
      Transaction tx;
      size_t blobSize;
      uint64_t fee;
      bool keptByBlock;
      time_t receiveTime;
    };

    std::list<cn::tx_memory_pool::TransactionDetails> getMemoryPool() const;

  private:

    struct TransactionPriorityComparator {
      // lhs > hrs
      bool operator()(const TransactionDetails& lhs, const TransactionDetails& rhs) const {
        // price(lhs) = lhs.fee / lhs.blobSize
        // price(lhs) > price(rhs) -->
        // lhs.fee / lhs.blobSize > rhs.fee / rhs.blobSize -->
        // lhs.fee * rhs.blobSize > rhs.fee * lhs.blobSize
        uint64_t lhs_hi, lhs_lo = mul128(lhs.fee, rhs.blobSize, &lhs_hi);
        uint64_t rhs_hi, rhs_lo = mul128(rhs.fee, lhs.blobSize, &rhs_hi);

        return
          // prefer more profitable transactions
          (lhs_hi >  rhs_hi) ||
          (lhs_hi == rhs_hi && lhs_lo >  rhs_lo) ||
          // prefer smaller
          (lhs_hi == rhs_hi && lhs_lo == rhs_lo && lhs.blobSize <  rhs.blobSize) ||
          // prefer older
          (lhs_hi == rhs_hi && lhs_lo == rhs_lo && lhs.blobSize == rhs.blobSize && lhs.receiveTime < rhs.receiveTime);
      }
    };

    typedef hashed_unique<BOOST_MULTI_INDEX_MEMBER(TransactionDetails, crypto::Hash, id)> main_index_t;
    typedef ordered_non_unique<identity<TransactionDetails>, TransactionPriorityComparator> fee_index_t;

    typedef multi_index_container<TransactionDetails,
      indexed_by<main_index_t, fee_index_t>
    > tx_container_t;

    typedef std::pair<uint64_t, uint64_t> GlobalOutput;
    typedef std::set<GlobalOutput> GlobalOutputsContainer;
    typedef std::unordered_map<crypto::KeyImage, std::unordered_set<crypto::Hash> > key_images_container;


    // double spending checking
    bool addTransactionInputs(const crypto::Hash& id, const Transaction& tx, bool keptByBlock);
    bool haveSpentInputs(const Transaction& tx) const;
    bool removeTransactionInputs(const crypto::Hash& id, const Transaction& tx, bool keptByBlock);

    tx_container_t::iterator removeTransaction(tx_container_t::iterator i);
    bool removeExpiredTransactions();
    bool is_transaction_ready_to_go(const Transaction& tx, TransactionCheckInfo& txd) const;
    void buildIndices();

    tools::ObserverManager<ITxPoolObserver> m_observerManager;
    const cn::Currency& m_currency;
    OnceInTimeInterval m_txCheckInterval;
    mutable std::recursive_mutex m_transactions_lock;
    key_images_container m_spent_key_images;
    GlobalOutputsContainer m_spentOutputs;

    std::string m_config_folder;
    cn::ITransactionValidator& m_validator;
    cn::ITimeProvider& m_timeProvider;

    tx_container_t m_transactions;  
    tx_container_t::nth_index<1>::type& m_fee_index;
    std::unordered_map<crypto::Hash, uint64_t> m_recentlyDeletedTransactions;

    logging::LoggerRef logger;

    PaymentIdIndex m_paymentIdIndex;
    TimestampTransactionsIndex m_timestampIndex;
    std::unordered_map<crypto::Hash, uint64_t> m_ttlIndex;
  };
}
