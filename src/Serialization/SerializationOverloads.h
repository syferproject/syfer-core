// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "ISerializer.h"

#include <array>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <set>
#include <type_traits>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <parallel_hashmap/phmap.h>

using phmap::flat_hash_map;
using phmap::parallel_flat_hash_map;
namespace cn
{

template <typename T>
typename std::enable_if<std::is_pod<T>::value>::type
serializeAsBinary(std::vector<T> &value, common::StringView name, cn::ISerializer &serializer)
{
  std::string blob;
  if (serializer.type() == ISerializer::INPUT)
  {
    serializer.binary(blob, name);

    const size_t blobSize = blob.size();

    value.resize(blobSize / sizeof(T));

    if (blobSize % sizeof(T) != 0)
    {
      throw std::runtime_error("Invalid blob size given!");
    }

    if (blobSize > 0)
    {
      memcpy(&value[0], blob.data(), blobSize);
    }
  }
else
{
  if (!value.empty())
  {
    blob.assign(reinterpret_cast<const char *>(&value[0]), value.size() * sizeof(T));
  }
  serializer.binary(blob, name);
}
} // namespace cn

template <typename T>
typename std::enable_if<std::is_pod<T>::value>::type
serializeAsBinary(std::list<T> &value, common::StringView name, cn::ISerializer &serializer)
{
  std::string blob;
  if (serializer.type() == ISerializer::INPUT)
  {
    serializer.binary(blob, name);

    uint64_t count = blob.size() / sizeof(T);

    if (blob.size() % sizeof(T) != 0)
    {
      throw std::runtime_error("Invalid blob size given!");
    }

    const T *ptr = reinterpret_cast<const T *>(blob.data());

    while (count--)
    {
      value.push_back(*ptr++);
    }
  }
  else
  {
    if (!value.empty())
    {
      blob.resize(value.size() * sizeof(T));
      T *ptr = reinterpret_cast<T *>(&blob[0]);

      for (const auto &item : value)
      {
        *ptr++ = item;
      }
    }
    serializer.binary(blob, name);
  }
}

template <typename Cont>
bool serializeContainer(Cont &value, common::StringView name, cn::ISerializer &serializer)
{
  size_t size = value.size();
  if (!serializer.beginArray(size, name))
  {
    value.clear();
    return false;
  }

  value.resize(size);

  for (auto &item : value)
  {
    serializer(const_cast<typename Cont::value_type &>(item), "");
  }

  serializer.endArray();
  return true;
}

template <typename E>
bool serializeEnumClass(E &value, common::StringView name, cn::ISerializer &serializer)
{
  static_assert(std::is_enum<E>::value, "E must be an enum class");

  typedef typename std::underlying_type<E>::type EType;

  if (serializer.type() == cn::ISerializer::INPUT)
  {
    EType numericValue;
    serializer(numericValue, name);
    value = static_cast<E>(numericValue);
  }
  else
  {
    auto numericValue = static_cast<EType>(value);
    serializer(numericValue, name);
  }

  return true;
}

template <typename T>
bool serialize(std::vector<T> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeContainer(value, name, serializer);
}

template <typename T>
bool serialize(std::list<T> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeContainer(value, name, serializer);
}

template <typename MapT, typename ReserveOp>
bool serializeMap(MapT &value, common::StringView name, cn::ISerializer &serializer, ReserveOp reserve)
{
  size_t size = value.size();

  if (!serializer.beginArray(size, name))
  {
    value.clear();
    return false;
  }

  if (serializer.type() == cn::ISerializer::INPUT)
  {
    reserve(size);

    for (size_t i = 0; i < size; ++i)
    {
      typename MapT::key_type key;
      typename MapT::mapped_type v;

      serializer.beginObject("");
      serializer(key, "key");
      serializer(v, "value");
      serializer.endObject();

      value.insert(std::make_pair(std::move(key), std::move(v)));
    }
  }
  else
  {
    for (auto &kv : value)
    {
      serializer.beginObject("");
      serializer(const_cast<typename MapT::key_type &>(kv.first), "key");
      serializer(kv.second, "value");
      serializer.endObject();
    }
  }

  serializer.endArray();
  return true;
}

template <typename SetT>
bool serializeSet(SetT &value, common::StringView name, cn::ISerializer &serializer)
{
  size_t size = value.size();

  if (!serializer.beginArray(size, name))
  {
    value.clear();
    return false;
  }

  if (serializer.type() == cn::ISerializer::INPUT)
  {
    for (size_t i = 0; i < size; ++i)
    {
      typename SetT::value_type key;
      serializer(key, "");
      value.insert(std::move(key));
    }
  }
  else
  {
    for (auto &key : value)
    {
      serializer(const_cast<typename SetT::value_type &>(key), "");
    }
  }

  serializer.endArray();
  return true;
}

template <typename K, typename Hash>
bool serialize(std::unordered_set<K, Hash> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeSet(value, name, serializer);
}

template <typename K, typename Cmp>
bool serialize(std::set<K, Cmp> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeSet(value, name, serializer);
}

template <typename K, typename V, typename Hash>
bool serialize(std::unordered_map<K, V, Hash> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeMap(value, name, serializer, [&value](size_t size) { value.reserve(size); });
}

template <typename K, typename V, typename Hash>
bool serialize(flat_hash_map<K, V, Hash> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeMap(value, name, serializer, [](size_t size) {});
}

template <typename K, typename V, typename Hash>
bool serialize(parallel_flat_hash_map<K, V, Hash> &value, common::StringView name, cn::ISerializer &serializer)
{
  return serializeMap(value, name, serializer, [](size_t size) {});
}

  template <typename K, typename V, typename Hash>
  bool serialize(std::unordered_multimap<K, V, Hash> & value, common::StringView name, cn::ISerializer & serializer)
  {
    return serializeMap(value, name, serializer, [&value](size_t size) { value.reserve(size); });
  }

  template <typename K, typename V, typename Hash>
  bool serialize(std::map<K, V, Hash> & value, common::StringView name, cn::ISerializer & serializer)
  {
    return serializeMap(value, name, serializer, [](size_t size) {});
  }

  template <typename K, typename V, typename Hash>
  bool serialize(std::multimap<K, V, Hash> & value, common::StringView name, cn::ISerializer & serializer)
  {
    return serializeMap(value, name, serializer, [](size_t size) {});
  }

  template <size_t size>
  bool serialize(std::array<uint8_t, size> & value, common::StringView name, cn::ISerializer & s)
  {
    return s.binary(value.data(), value.size(), name);
  }

  template <typename T1, typename T2>
  void serialize(std::pair<T1, T2> & value, ISerializer & s)
  {
    s(value.first, "first");
    s(value.second, "second");
  }

  template <typename Element, typename Iterator>
  void writeSequence(Iterator begin, Iterator end, common::StringView name, ISerializer & s)
  {
    size_t size = std::distance(begin, end);
    s.beginArray(size, name);
    for (Iterator i = begin; i != end; ++i)
    {
      s(const_cast<Element &>(*i), "");
    }
    s.endArray();
  }

  template <typename Element, typename Iterator>
  void readSequence(Iterator outputIterator, common::StringView name, ISerializer & s)
  {
    size_t size = 0;
    s.beginArray(size, name);

    while (size--)
    {
      Element e;
      s(e, "");
      *outputIterator++ = std::move(e);
    }

    s.endArray();
  }

  //convinience function since we change block height type
  void serializeBlockHeight(ISerializer & s, uint32_t & blockHeight, common::StringView name);

  //convinience function since we change global output index type
  void serializeGlobalOutputIndex(ISerializer & s, uint32_t & globalOutputIndex, common::StringView name);

} // namespace cn
