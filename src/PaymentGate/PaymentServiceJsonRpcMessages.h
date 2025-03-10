// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <exception>
#include <limits>
#include <vector>
#include "IWallet.h"
#include "Serialization/ISerializer.h"

namespace payment_service
{

class RequestSerializationError : public std::exception
{
public:
  const char *what() const throw() override { return "Request error"; }
};

struct Save
{
  struct Request
  {
    void serialize(const cn::ISerializer &serializer) const;
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct Reset {
  struct Request {
    std::string viewSecretKey;
    uint32_t scanHeight = std::numeric_limits<uint32_t>::max();

    void serialize(cn::ISerializer& serializer);
  };

  struct Response {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct ExportWallet
{
  struct Request
  {
    std::string exportFilename;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct ExportWalletKeys
{
  struct Request
  {
    std::string exportFilename;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct GetViewKey
{
  struct Request
  {
    void serialize(const cn::ISerializer &serializer) const;
  };

  struct Response
  {
    std::string viewSecretKey;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetStatus
{
  struct Request
  {
    void serialize(const cn::ISerializer &serializer) const;
  };

  struct Response
  {
    uint32_t blockCount;
    uint32_t knownBlockCount;
    std::string lastBlockHash;
    uint32_t peerCount;
    uint32_t depositCount;
    uint32_t transactionCount;
    uint32_t addressCount;

    void serialize(cn::ISerializer &serializer);
  };
};

struct CreateDeposit
{
  struct Request
  {

    uint64_t amount;
    uint64_t term;
    std::string sourceAddress;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };
};

struct WithdrawDeposit
{
  struct Request
  {

    uint64_t depositId;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };
};

struct SendDeposit
{
  struct Request
  {

    uint64_t amount;
    uint64_t term;
    std::string sourceAddress;
    std::string destinationAddress;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetDeposit
{
  struct Request
  {
    size_t depositId;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    uint64_t amount;
    uint64_t term;
    uint64_t interest;
    uint64_t height;
    uint64_t unlockHeight;
    std::string creatingTransactionHash;
    std::string spendingTransactionHash;
    bool locked;
    std::string address;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetAddresses
{
  struct Request
  {
    void serialize(const cn::ISerializer &serializer) const;
  };

  struct Response
  {
    std::vector<std::string> addresses;

    void serialize(cn::ISerializer &serializer);
  };
};

struct CreateAddress
{
  struct Request
  {
    std::string spendSecretKey;
    std::string spendPublicKey;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string address;

    void serialize(cn::ISerializer &serializer);
  };
};

struct CreateAddressList
{
  struct Request
  {
    std::vector<std::string> spendSecretKeys;
    bool reset = false;
    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<std::string> addresses;
    void serialize(cn::ISerializer &serializer);
  };
};

struct DeleteAddress
{
  struct Request
  {
    std::string address;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct GetSpendKeys
{
  struct Request
  {
    std::string address;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string spendSecretKey;
    std::string spendPublicKey;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetBalance
{
  struct Request
  {
    std::string address;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    uint64_t availableBalance;
    uint64_t lockedAmount;
    uint64_t lockedDepositBalance;
    uint64_t unlockedDepositBalance;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetBlockHashes
{
  struct Request
  {
    uint32_t firstBlockIndex;
    uint32_t blockCount;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<std::string> blockHashes;

    void serialize(cn::ISerializer &serializer);
  };
};

struct TransactionHashesInBlockRpcInfo
{
  std::string blockHash;
  std::vector<std::string> transactionHashes;

  void serialize(cn::ISerializer &serializer);
};

struct GetTransactionHashes
{
  struct Request
  {
    std::vector<std::string> addresses;
    std::string blockHash;
    uint32_t firstBlockIndex = std::numeric_limits<uint32_t>::max();
    uint32_t blockCount;
    std::string paymentId;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<TransactionHashesInBlockRpcInfo> items;

    void serialize(cn::ISerializer &serializer);
  };
};

struct CreateIntegrated
{
  struct Request
  {
    std::string address;
    std::string payment_id;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string integrated_address;
    void serialize(cn::ISerializer &serializer);
  };
};

struct SplitIntegrated
{
  struct Request
  {
    std::string integrated_address;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string address;
    std::string payment_id;

    void serialize(cn::ISerializer &serializer);
  };
};

struct TransferRpcInfo
{
  uint8_t type;
  std::string address;
  int64_t amount;
  std::string message;

  void serialize(cn::ISerializer &serializer);
};

struct TransactionRpcInfo
{
  uint8_t state;
  std::string transactionHash;
  uint32_t blockIndex;
  uint64_t timestamp;
  uint32_t confirmations = 0;
  bool isBase;
  uint64_t unlockTime;
  int64_t amount;
  uint64_t fee;
  std::vector<TransferRpcInfo> transfers;
  std::string extra;
  std::string paymentId;
  size_t firstDepositId = cn::WALLET_INVALID_DEPOSIT_ID;
  size_t depositCount = 0;

  void serialize(cn::ISerializer &serializer);
};

struct GetTransaction
{
  struct Request
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    TransactionRpcInfo transaction;

    void serialize(cn::ISerializer &serializer);
  };
};

struct TransactionsInBlockRpcInfo
{
  std::string blockHash;
  std::vector<TransactionRpcInfo> transactions;

  void serialize(cn::ISerializer &serializer);
};

struct GetTransactions
{
  struct Request
  {
    std::vector<std::string> addresses;
    std::string blockHash;
    uint32_t firstBlockIndex = std::numeric_limits<uint32_t>::max();
    uint32_t blockCount;
    std::string paymentId;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<TransactionsInBlockRpcInfo> items;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetUnconfirmedTransactionHashes
{
  struct Request
  {
    std::vector<std::string> addresses;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<std::string> transactionHashes;

    void serialize(cn::ISerializer &serializer);
  };
};

struct WalletRpcOrder
{
  std::string address;
  uint64_t amount;
  std::string message;

  void serialize(cn::ISerializer &serializer);
};

struct WalletRpcMessage
{
  std::string address;
  std::string message;

  void serialize(cn::ISerializer &serializer);
};

struct SendTransaction
{
  struct Request
  {
    std::vector<std::string> sourceAddresses;
    std::vector<WalletRpcOrder> transfers;
    std::string changeAddress;
    uint64_t fee = 1000;
    uint32_t anonymity = cn::parameters::MINIMUM_MIXIN;
    std::string extra;
    std::string paymentId;
    uint64_t unlockTime = 0;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;
    std::string transactionSecretKey;

    void serialize(cn::ISerializer &serializer);
  };
};

struct CreateDelayedTransaction
{
  struct Request
  {
    std::vector<std::string> addresses;
    std::vector<WalletRpcOrder> transfers;
    std::string changeAddress;
    uint64_t fee = 1000;
    uint32_t anonymity = cn::parameters::MINIMUM_MIXIN;
    std::string extra;
    std::string paymentId;
    uint64_t unlockTime = 0;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };
};

struct GetDelayedTransactionHashes
{
  struct Request
  {
    void serialize(const cn::ISerializer &serializer) const;
  };

  struct Response
  {
    std::vector<std::string> transactionHashes;

    void serialize(cn::ISerializer &serializer);
  };
};

struct DeleteDelayedTransaction
{
  struct Request
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct SendDelayedTransaction
{
  struct Request
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    void serialize(const cn::ISerializer &serializer) const;
  };
};

struct GetMessagesFromExtra
{
  struct Request
  {
    std::string extra;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::vector<std::string> messages;

    void serialize(cn::ISerializer &serializer);
  };
};

struct EstimateFusion
{
  struct Request
  {
    uint64_t threshold;
    std::vector<std::string> addresses;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    uint32_t fusionReadyCount;
    uint32_t totalOutputCount;

    void serialize(cn::ISerializer &serializer);
  };
};

struct SendFusionTransaction
{
  struct Request
  {
    uint64_t threshold;
    uint32_t anonymity = 0;
    std::vector<std::string> addresses;
    std::string destinationAddress;

    void serialize(cn::ISerializer &serializer);
  };

  struct Response
  {
    std::string transactionHash;

    void serialize(cn::ISerializer &serializer);
  };
};

} //namespace payment_service
