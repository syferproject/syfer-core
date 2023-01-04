// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <ostream>
#include "IOutputStream.h"

namespace common {

class StdOutputStream : public IOutputStream {
public:
  StdOutputStream(std::ostream& out);
  StdOutputStream& operator=(const StdOutputStream&) = delete;
  size_t writeSome(const void* data, size_t size) override;

private:
  std::ostream& out;
};

}
