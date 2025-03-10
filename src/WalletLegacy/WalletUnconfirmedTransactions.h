// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "IWalletLegacy.h"
#include "ITransfersContainer.h"

#include <unordered_map>
#include <unordered_set>
#include <time.h>
#include <boost/functional/hash.hpp>

#include "crypto/crypto.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "WalletLegacy/WalletUnconfirmedTransactions.h"

namespace cn {
class ISerializer;

typedef std::pair<crypto::PublicKey, size_t> TransactionOutputId;
}

namespace std {

template<> 
struct hash<cn::TransactionOutputId> {
  size_t operator()(const cn::TransactionOutputId &_v) const {    
    return hash<crypto::PublicKey>()(_v.first) ^ _v.second;
  } 
}; 

}

namespace cn {


struct UnconfirmedTransferDetails {

  UnconfirmedTransferDetails() :
    amount(0), sentTime(0), transactionId(WALLET_LEGACY_INVALID_TRANSACTION_ID) {}

  cn::Transaction tx;
  uint64_t amount;
  uint64_t outsAmount;
  time_t sentTime;
  TransactionId transactionId;
  std::vector<TransactionOutputId> usedOutputs;
};

struct UnconfirmedSpentDepositDetails {
  TransactionId transactionId;
  uint64_t depositsSum;
  uint64_t fee;
};

class WalletUnconfirmedTransactions
{
public:

  explicit WalletUnconfirmedTransactions(uint64_t uncofirmedTransactionsLiveTime);

  bool serialize(cn::ISerializer& s);
  bool deserializeV1(cn::ISerializer& s);

  bool findTransactionId(const crypto::Hash& hash, TransactionId& id);
  void erase(const crypto::Hash& hash);
  void add(const cn::Transaction& tx, TransactionId transactionId, 
    uint64_t amount, const std::vector<TransactionOutputInformation>& usedOutputs);
  void updateTransactionId(const crypto::Hash& hash, TransactionId id);

  void addCreatedDeposit(DepositId id, uint64_t totalAmount);
  void addDepositSpendingTransaction(const crypto::Hash& transactionHash, const UnconfirmedSpentDepositDetails& details);

  void eraseCreatedDeposit(DepositId id);

  uint64_t countCreatedDepositsSum() const;
  uint64_t countSpentDepositsProfit() const;
  uint64_t countSpentDepositsTotalAmount() const;

  uint64_t countUnconfirmedOutsAmount() const;
  uint64_t countUnconfirmedTransactionsAmount() const;
  bool isUsed(const TransactionOutputInformation& out) const;
  void reset();

  std::vector<TransactionId> deleteOutdatedTransactions();

private:

  void collectUsedOutputs();
  void deleteUsedOutputs(const std::vector<TransactionOutputId>& usedOutputs);

  bool eraseUnconfirmedTransaction(const crypto::Hash& hash);
  bool eraseDepositSpendingTransaction(const crypto::Hash& hash);

  bool findUnconfirmedTransactionId(const crypto::Hash& hash, TransactionId& id);
  bool findUnconfirmedDepositSpendingTransactionId(const crypto::Hash& hash, TransactionId& id);

  typedef std::unordered_map<crypto::Hash, UnconfirmedTransferDetails, boost::hash<crypto::Hash>> UnconfirmedTxsContainer;
  typedef std::unordered_set<TransactionOutputId> UsedOutputsContainer;

  UnconfirmedTxsContainer m_unconfirmedTxs;
  UsedOutputsContainer m_usedOutputs;
  uint64_t m_uncofirmedTransactionsLiveTime;

  std::unordered_map<DepositId, uint64_t> m_createdDeposits;
  std::unordered_map<crypto::Hash, UnconfirmedSpentDepositDetails> m_spentDeposits;
};

} // namespace cn
