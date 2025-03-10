// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "JsonInputValueSerializer.h"

#include <cassert>
#include <stdexcept>

#include "Common/StringTools.h"

using common::JsonValue;
using namespace cn;

class serializer_error : public std::runtime_error
{
public:
  explicit serializer_error(const std::string &s) : std::runtime_error("This type of serialization is not supported: " + s){};
};

JsonInputValueSerializer::JsonInputValueSerializer(const common::JsonValue& value) {
  if (!value.isObject()) {
    throw serializer_error("Object expected.");
  }

  chain.push_back(&value);
}

JsonInputValueSerializer::JsonInputValueSerializer(common::JsonValue&& value) : root(std::move(value)) {
  if (!this->root.isObject()) {
    throw serializer_error("Object expected.");
  }

  chain.push_back(&this->root);
}

ISerializer::SerializerType JsonInputValueSerializer::type() const {
  return ISerializer::INPUT;
}

bool JsonInputValueSerializer::beginObject(common::StringView name) {
  const JsonValue* parent = chain.back();

  if (parent->isArray()) {
    const JsonValue& v = (*parent)[idxs.back()++];
    chain.push_back(&v);
    return true;
  }

  if (parent->contains(std::string(name))) {
    const JsonValue& v = (*parent)(std::string(name));
    chain.push_back(&v);
    return true;
  }

  return false;
}

void JsonInputValueSerializer::endObject() {
  assert(!chain.empty());
  chain.pop_back();
}

bool JsonInputValueSerializer::beginArray(size_t& size, common::StringView name) {
  const JsonValue* parent = chain.back();
  std::string strName(name);

  if (parent->contains(strName)) {
    const JsonValue& arr = (*parent)(strName);
    size = arr.size();
    chain.push_back(&arr);
    idxs.push_back(0);
    return true;
  }
 
  size = 0;
  return false;
}

void JsonInputValueSerializer::endArray() {
  assert(!chain.empty());
  assert(!idxs.empty());

  chain.pop_back();
  idxs.pop_back();
}

bool JsonInputValueSerializer::operator()(uint16_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int16_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint32_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int32_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int64_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint64_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(double& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint8_t& value, common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(std::string& value, common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }
  value = ptr->getString();
  return true;
}

bool JsonInputValueSerializer::operator()(bool& value, common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }
  value = ptr->getBool();
  return true;
}

bool JsonInputValueSerializer::binary(void* value, size_t size, common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }

  common::fromHex(ptr->getString(), value, size);
  return true;
}

bool JsonInputValueSerializer::binary(std::string& value, common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }

  std::string valueHex = ptr->getString();
  value = common::asString(common::fromHex(valueHex));

  return true;
}

const JsonValue* JsonInputValueSerializer::getValue(common::StringView name) {
  const JsonValue& val = *chain.back();
  if (val.isArray()) {
    return &val[idxs.back()++];
  }

  std::string strName(name);
  return val.contains(strName) ? &val(strName) : nullptr;
}
