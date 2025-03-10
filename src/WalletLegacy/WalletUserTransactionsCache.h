// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <deque>

#include <boost/functional/hash.hpp>

#include "crypto/hash.h"
#include "IWalletLegacy.h"
#include "ITransfersContainer.h"

#include "CryptoNoteCore/Currency.h"
#include "WalletLegacy/WalletDepositInfo.h"
#include "WalletLegacy/WalletLegacyEvent.h"
#include "WalletLegacy/WalletUnconfirmedTransactions.h"

namespace cn {
class ISerializer;
}

namespace std {
  template<>
  struct hash<std::tuple<crypto::Hash, uint32_t>> {
    size_t operator()(const std::tuple<crypto::Hash, uint32_t>& item) const {
      size_t hash = 0;
      boost::hash_combine(hash, std::get<0>(item));
      boost::hash_combine(hash, std::get<1>(item));
      return hash;
    }
  };
}

namespace cn {

using UserDeposits = std::vector<DepositInfo>;

class WalletUserTransactionsCache
{
public:
  explicit WalletUserTransactionsCache(uint64_t mempoolTxLiveTime = 60 * 60 * 24);

  bool serialize(cn::ISerializer& serializer);
  void deserializeLegacyV1(cn::ISerializer& serializer);

  uint64_t unconfirmedTransactionsAmount() const;
  uint64_t unconfrimedOutsAmount() const;
  uint64_t countUnconfirmedCreatedDepositsSum() const;
  uint64_t countUnconfirmedSpentDepositsProfit() const;
  uint64_t countUnconfirmedSpentDepositsTotalAmount() const;

  size_t getTransactionCount() const;
  size_t getTransferCount() const;
  size_t getDepositCount() const;

  TransactionId addNewTransaction(uint64_t amount,
                                  uint64_t fee,
                                  const std::string& extra,
                                  const std::vector<WalletLegacyTransfer>& transfers,
                                  uint64_t unlockTime,
                                  const std::vector<TransactionMessage>& messages);
  void updateTransaction(TransactionId transactionId,
                         const cn::Transaction& tx,
                         uint64_t amount,
                         const std::vector<TransactionOutputInformation>& usedOutputs);
  void updateTransactionSendingState(TransactionId transactionId, std::error_code ec);

  void addCreatedDeposit(DepositId id, uint64_t totalAmount);
  void addDepositSpendingTransaction(const crypto::Hash& transactionHash, const UnconfirmedSpentDepositDetails& details);

  std::deque<std::unique_ptr<WalletLegacyEvent>> onTransactionUpdated(const TransactionInformation& txInfo,
                                                                      int64_t txBalance,
                                                                      const std::vector<TransactionOutputInformation>& newDeposits,
                                                                      const std::vector<TransactionOutputInformation>& spentDeposits,
                                                                      const Currency& currency);
  std::deque<std::unique_ptr<WalletLegacyEvent>> onTransactionDeleted(const crypto::Hash& transactionHash);

  std::vector<DepositId> unlockDeposits(const std::vector<TransactionOutputInformation>& transfers);
  std::vector<DepositId> lockDeposits(const std::vector<TransactionOutputInformation>& transfers);

  TransactionId findTransactionByTransferId(TransferId transferId) const;

  bool getTransaction(TransactionId transactionId, WalletLegacyTransaction& transaction) const;
  WalletLegacyTransaction& getTransaction(TransactionId transactionId);
  bool getTransfer(TransferId transferId, WalletLegacyTransfer& transfer) const;
  WalletLegacyTransfer& getTransfer(TransferId transferId);
  bool getDeposit(DepositId depositId, Deposit& deposit) const;
  Deposit& getDeposit(DepositId depositId);

  bool isUsed(const TransactionOutputInformation& out) const;
  void reset();

  std::vector<TransactionId> deleteOutdatedTransactions();

  DepositId insertDeposit(const Deposit& deposit, size_t depositIndexInTransaction, const crypto::Hash& transactionHash);
  bool getDepositInTransactionInfo(DepositId depositId, crypto::Hash& transactionHash, uint32_t& outputInTransaction);

  std::vector<Payments> getTransactionsByPaymentIds(const std::vector<PaymentId>& paymentIds) const;
  TransactionId findTransactionByHash(const crypto::Hash& hash);
private:


  TransactionId insertTransaction(WalletLegacyTransaction&& Transaction);
  TransferId insertTransfers(const std::vector<WalletLegacyTransfer>& transfers);
  void updateUnconfirmedTransactions();

  void restoreTransactionOutputToDepositIndex();
  std::vector<DepositId> createNewDeposits(TransactionId creatingTransactionId,
                                           const std::vector<TransactionOutputInformation>& depositOutputs,
                                           const Currency& currency,
										   uint32_t height);
  DepositId insertNewDeposit(const TransactionOutputInformation& depositOutput,
                             TransactionId creatingTransactionId,
                             const Currency& currency, uint32_t height);
  std::vector<DepositId> processSpentDeposits(TransactionId spendingTransactionId, const std::vector<TransactionOutputInformation>& spentDepositOutputs);
  DepositId getDepositId(const crypto::Hash& creatingTransactionHash, uint32_t outputInTransaction);

  std::vector<DepositId> getDepositIdsBySpendingTransaction(TransactionId transactionId);

  void eraseCreatedDeposit(DepositId id);

  using UserTransfers = std::vector<WalletLegacyTransfer>;
  using UserTransactions = std::vector<WalletLegacyTransaction>;
  using Offset = UserTransactions::size_type;
  using UserPaymentIndex = std::unordered_map<PaymentId, std::vector<Offset>, boost::hash<PaymentId>>;

  void rebuildPaymentsIndex();
  void pushToPaymentsIndex(const PaymentId& paymentId, Offset distance);
  void pushToPaymentsIndexInternal(Offset distance, const WalletLegacyTransaction& info, std::vector<uint8_t>& extra);
  void popFromPaymentsIndex(const PaymentId& paymentId, Offset distance);

  UserTransactions m_transactions;
  UserTransfers m_transfers;
  UserDeposits m_deposits;
  WalletUnconfirmedTransactions m_unconfirmedTransactions;
  //tuple<Creating transaction hash, outputIndexInTransaction> -> depositId
  std::unordered_map<std::tuple<crypto::Hash, uint32_t>, DepositId> m_transactionOutputToDepositIndex;
  UserPaymentIndex m_paymentsIndex;
};

} //namespace cn
