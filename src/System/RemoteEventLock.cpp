// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "RemoteEventLock.h"
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <System/Dispatcher.h>
#include <System/Event.h>

namespace platform_system {

RemoteEventLock::RemoteEventLock(Dispatcher& dispatcher, Event& event) : dispatcher(dispatcher), event(event) {
  std::mutex mutex;
  std::condition_variable condition;
  bool locked = false;

  dispatcher.remoteSpawn([&]() {
    while (!event.get()) {
      event.wait();
    }

    event.clear();
    mutex.lock();
    locked = true;
    condition.notify_one();
    mutex.unlock();
  });

  std::unique_lock<std::mutex> lock(mutex);
  while (!locked) {
    condition.wait(lock);
  }
}

RemoteEventLock::~RemoteEventLock() {
  Event* eventPointer = &event;
  dispatcher.remoteSpawn([=]() {
    assert(!eventPointer->get());
    eventPointer->set();
  });
}

}
