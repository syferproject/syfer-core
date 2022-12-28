// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Syfer Devs
// Copyright (c) 2018-2022 Syfer Network & Syfer Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <functional>

namespace tools {
  
  class SignalHandler
  {
  public:
    static bool install(std::function<void(void)> t);
  };
}
