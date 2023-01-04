// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "StringInputStream.h"
#include <string.h>

namespace common {

StringInputStream::StringInputStream(const std::string& in) : in(in), offset(0) {
}

size_t StringInputStream::readSome(void* data, size_t size) {
  if (size > in.size() - offset) {
    size = in.size() - offset;
  }

  memcpy(data, in.data() + offset, size);
  offset += size;
  return size;
}

}
