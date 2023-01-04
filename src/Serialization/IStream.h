// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <memory>

namespace cn {

class IInputStream {
public:
  virtual size_t read(char* data, size_t size) = 0;
  virtual ~IInputStream() = default;
};

class IOutputStream {
public:
  virtual void write(const char* data, size_t size) = 0;
  virtual ~IOutputStream() = default;
};

}
