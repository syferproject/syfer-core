// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "P2pContextOwner.h"
#include <cassert>
#include "P2pContext.h"

namespace cn {

P2pContextOwner::P2pContextOwner(P2pContext* ctx, ContextList& contextList) : contextList(contextList) {
  contextIterator = contextList.insert(contextList.end(), ContextList::value_type(ctx));
}

P2pContextOwner::P2pContextOwner(P2pContextOwner&& other) : contextList(other.contextList), contextIterator(other.contextIterator) {
  other.contextIterator = contextList.end();
}

P2pContextOwner::~P2pContextOwner() {
  if (contextIterator != contextList.end()) {
    contextList.erase(contextIterator);
  }
}

P2pContext& P2pContextOwner::get() {
  assert(contextIterator != contextList.end());
  return *contextIterator->get();
}

P2pContext* P2pContextOwner::operator -> () {
  return &get();
}

}
