// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <ostream>
#include <istream>

#include "crypto/hash.h"
#include "crypto/chacha8.h"

namespace cn {
class AccountBase;
class ISerializer;
}

namespace cn {

class WalletUserTransactionsCache;

class WalletLegacySerializer {
public:
  WalletLegacySerializer(cn::AccountBase& account, WalletUserTransactionsCache& transactionsCache);

  void serialize(std::ostream& stream, const std::string& password, bool saveDetailed, const std::string& cache);
  void deserialize(std::istream& stream, const std::string& password, std::string& cache);
  bool deserialize(std::istream& stream, const std::string& password);  

private:
  void saveKeys(cn::ISerializer& serializer);
  void loadKeys(cn::ISerializer& serializer);

  crypto::chacha8_iv encrypt(const std::string& plain, const std::string& password, std::string& cipher);
  void decrypt(const std::string& cipher, std::string& plain, crypto::chacha8_iv iv, const std::string& password);

  cn::AccountBase& account;
  WalletUserTransactionsCache& transactionsCache;
  const uint32_t walletSerializationVersion;
};

} //namespace cn
