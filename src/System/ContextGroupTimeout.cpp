// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ContextGroupTimeout.h"
#include <System/InterruptedException.h>

using namespace platform_system;

ContextGroupTimeout::ContextGroupTimeout(Dispatcher& dispatcher, ContextGroup& contextGroup, std::chrono::nanoseconds timeout) :
  workingContextGroup(dispatcher),
  timeoutTimer(dispatcher) {
  workingContextGroup.spawn([&, timeout] {
    try {
      timeoutTimer.sleep(timeout);
      contextGroup.interrupt();
    } catch (InterruptedException&) {
    }
  });
}
