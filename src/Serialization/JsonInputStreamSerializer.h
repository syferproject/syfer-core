// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include "../Common/JsonValue.h"
#include "JsonInputValueSerializer.h"

namespace cn {

//deserialization
class JsonInputStreamSerializer : public JsonInputValueSerializer {
public:
  explicit JsonInputStreamSerializer(std::istream& stream);
  ~JsonInputStreamSerializer() override = default;
};

}
