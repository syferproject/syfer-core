// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <limits>
#include "Common/MemoryInputStream.h"
#include "Common/StringTools.h"
#include "Common/VectorOutputStream.h"
#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/BinaryInputStreamSerializer.h"
#include "CryptoNoteSerialization.h"

namespace cn {

void getBinaryArrayHash(const BinaryArray& binaryArray, crypto::Hash& hash);
crypto::Hash getBinaryArrayHash(const BinaryArray& binaryArray);

template<class T>
bool toBinaryArray(const T& object, BinaryArray& binaryArray) {
  try {
    ::common::VectorOutputStream stream(binaryArray);
    BinaryOutputStreamSerializer serializer(stream);
    serialize(const_cast<T&>(object), serializer);
  } catch (std::exception&) {
    return false;
  }

  return true;
}

template<>
bool toBinaryArray(const BinaryArray& object, BinaryArray& binaryArray); 

template<class T>
BinaryArray toBinaryArray(const T& object) {
  BinaryArray ba;
  toBinaryArray(object, ba);
  return ba;
}

template<class T>
bool fromBinaryArray(T& object, const BinaryArray& binaryArray) {
  bool result = false;
  try {
    common::MemoryInputStream stream(binaryArray.data(), binaryArray.size());
    BinaryInputStreamSerializer serializer(stream);
    serialize(object, serializer);
    result = stream.endOfStream(); // check that all data was consumed
  } catch (std::exception&) {
    return result;
  }

  return result;
}

template<class T>
bool getObjectBinarySize(const T& object, size_t& size) {
  BinaryArray ba;
  if (!toBinaryArray(object, ba)) {
    size = (std::numeric_limits<size_t>::max)();
    return false;
  }

  size = ba.size();
  return true;
}

template<class T>
size_t getObjectBinarySize(const T& object) {
  size_t size;
  getObjectBinarySize(object, size);
  return size;
}

template<class T>
bool getObjectHash(const T& object, crypto::Hash& hash) {
  BinaryArray ba;
  if (!toBinaryArray(object, ba)) {
    hash = NULL_HASH;
    return false;
  }

  hash = getBinaryArrayHash(ba);
  return true;
}

template<class T>
bool getObjectHash(const T& object, crypto::Hash& hash, size_t& size) {
  BinaryArray ba;
  if (!toBinaryArray(object, ba)) {
    hash = NULL_HASH;
    size = (std::numeric_limits<size_t>::max)();
    return false;
  }

  size = ba.size();
  hash = getBinaryArrayHash(ba);
  return true;
}

template<class T>
crypto::Hash getObjectHash(const T& object) {
  crypto::Hash hash;
  getObjectHash(object, hash);
  return hash;
}

uint64_t getInputAmount(const Transaction& transaction);
std::vector<uint64_t> getInputsAmounts(const Transaction& transaction);
uint64_t getOutputAmount(const Transaction& transaction);
void decomposeAmount(uint64_t amount, uint64_t dustThreshold, std::vector<uint64_t>& decomposedAmounts);
}
