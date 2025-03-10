// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/utility/value_init.hpp>
#include <CryptoNote.h>
#include "CryptoNoteBasic.h"
#include "CryptoNoteSerialization.h"

#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/BinaryInputStreamSerializer.h"

namespace logging {
class ILogger;
}

namespace cn {

bool parseAndValidateTransactionFromBinaryArray(const BinaryArray& transactionBinaryArray, Transaction& transaction, crypto::Hash& transactionHash, crypto::Hash& transactionPrefixHash);

struct TransactionSourceEntry {
  typedef std::pair<uint32_t, crypto::PublicKey> OutputEntry;

  std::vector<OutputEntry> outputs;           //index + key
  size_t realOutput;                          //index in outputs vector of real output_entry
  crypto::PublicKey realTransactionPublicKey; //incoming real tx public key
  size_t realOutputIndexInTransaction;        //index in transaction outputs vector
  uint64_t amount;                            //money
};

struct TransactionDestinationEntry {
  uint64_t amount;                    //money
  AccountPublicAddress addr;          //destination address

  TransactionDestinationEntry() : amount(0), addr(boost::value_initialized<AccountPublicAddress>()) {}
  TransactionDestinationEntry(uint64_t amount, const AccountPublicAddress &addr) : amount(amount), addr(addr) {}
};

struct tx_message_entry
{
  std::string message;
  bool encrypt;
  AccountPublicAddress addr;
};

bool generateDeterministicTransactionKeys(const crypto::Hash &inputsHash, const crypto::SecretKey &viewSecretKey, KeyPair &generatedKeys);
bool generateDeterministicTransactionKeys(const Transaction &tx, const crypto::SecretKey &viewSecretKey, KeyPair &generatedKeys);

bool constructTransaction(
  const AccountKeys& senderAccountKeys,
  const std::vector<TransactionSourceEntry>& sources,
  const std::vector<TransactionDestinationEntry>& destinations,
  const std::vector<tx_message_entry>& messages,
  uint64_t ttl, std::vector<uint8_t> extra, Transaction& transaction, uint64_t unlock_time, logging::ILogger& log, crypto::SecretKey& transactionSK);

inline bool constructTransaction(
  const AccountKeys& sender_account_keys,
  const std::vector<TransactionSourceEntry>& sources,
  const std::vector<TransactionDestinationEntry>& destinations,
  std::vector<uint8_t> extra, Transaction& tx, uint64_t unlock_time, logging::ILogger& log, crypto::SecretKey& transactionSK) {

  return constructTransaction(sender_account_keys, sources, destinations, std::vector<tx_message_entry>(), 0, extra, tx, unlock_time, log, transactionSK);
}

bool is_out_to_acc(const AccountKeys& acc, const KeyOutput& out_key, const crypto::PublicKey& tx_pub_key, size_t keyIndex);
bool is_out_to_acc(const AccountKeys& acc, const KeyOutput& out_key, const crypto::KeyDerivation& derivation, size_t keyIndex);
bool lookup_acc_outs(const AccountKeys& acc, const Transaction& tx, const crypto::PublicKey& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered);
bool lookup_acc_outs(const AccountKeys& acc, const Transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered);
bool generate_key_image_helper(const AccountKeys& ack, const crypto::PublicKey& tx_public_key, size_t real_output_index, KeyPair& in_ephemeral, crypto::KeyImage& ki);
std::string short_hash_str(const crypto::Hash& h);

bool get_block_hashing_blob(const Block& b, BinaryArray& blob);
bool get_aux_block_header_hash(const Block& b, crypto::Hash& res);
bool get_block_hash(const Block& b, crypto::Hash& res);
crypto::Hash get_block_hash(const Block& b);
bool get_block_longhash(crypto::cn_context &context, const Block& b, crypto::Hash& res);
bool get_inputs_money_amount(const Transaction& tx, uint64_t& money);
uint64_t get_outs_money_amount(const Transaction& tx);
bool check_inputs_types_supported(const TransactionPrefix& tx);
bool check_outs_valid(const TransactionPrefix& tx, std::string* error = 0);
bool checkMultisignatureInputsDiff(const TransactionPrefix& tx);

bool check_money_overflow(const TransactionPrefix& tx);
bool check_outs_overflow(const TransactionPrefix& tx);
bool check_inputs_overflow(const TransactionPrefix& tx);
uint32_t get_block_height(const Block& b);
std::vector<uint32_t> relative_output_offsets_to_absolute(const std::vector<uint32_t>& off);
std::vector<uint32_t> absolute_output_offsets_to_relative(const std::vector<uint32_t>& off);


// 62387455827 -> 455827 + 7000000 + 80000000 + 300000000 + 2000000000 + 60000000000, where 455827 <= dust_threshold
template<typename chunk_handler_t, typename dust_handler_t>
void decompose_amount_into_digits(uint64_t amount, uint64_t dust_threshold, const chunk_handler_t& chunk_handler, const dust_handler_t& dust_handler) {
  if (0 == amount) {
    return;
  }

  bool is_dust_handled = false;
  uint64_t dust = 0;
  uint64_t order = 1;
  while (0 != amount) {
    uint64_t chunk = (amount % 10) * order;
    amount /= 10;
    order *= 10;

    if (dust + chunk <= dust_threshold) {
      dust += chunk;
    } else {
      if (!is_dust_handled && 0 != dust) {
        dust_handler(dust);
        is_dust_handled = true;
      }
      if (0 != chunk) {
        chunk_handler(chunk);
      }
    }
  }

  if (!is_dust_handled && 0 != dust) {
    dust_handler(dust);
  }
}

void get_tx_tree_hash(const std::vector<crypto::Hash>& tx_hashes, crypto::Hash& h);
crypto::Hash get_tx_tree_hash(const std::vector<crypto::Hash>& tx_hashes);
crypto::Hash get_tx_tree_hash(const Block& b);
bool is_valid_decomposed_amount(uint64_t amount);


}
