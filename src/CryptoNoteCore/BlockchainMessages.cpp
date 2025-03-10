// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "CryptoNoteCore/BlockchainMessages.h"

namespace cn {

NewBlockMessage::NewBlockMessage(const crypto::Hash& hash) : blockHash(hash) {}

void NewBlockMessage::get(crypto::Hash& hash) const {
  hash = blockHash;
}

NewAlternativeBlockMessage::NewAlternativeBlockMessage(const crypto::Hash& hash) : blockHash(hash) {}

void NewAlternativeBlockMessage::get(crypto::Hash& hash) const {
  hash = blockHash;
}

ChainSwitchMessage::ChainSwitchMessage(std::vector<crypto::Hash>&& hashes) : blocksFromCommonRoot(std::move(hashes)) {}

ChainSwitchMessage::ChainSwitchMessage(const ChainSwitchMessage& other) : blocksFromCommonRoot(other.blocksFromCommonRoot) {}

void ChainSwitchMessage::get(std::vector<crypto::Hash>& hashes) const {
  hashes = blocksFromCommonRoot;
}

BlockchainMessage::BlockchainMessage(NewBlockMessage&& message) : type(MessageType::NEW_BLOCK_MESSAGE), newBlockMessage(std::move(message)) {}

BlockchainMessage::BlockchainMessage(NewAlternativeBlockMessage&& message) : type(MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE), newAlternativeBlockMessage(std::move(message)) {}

BlockchainMessage::BlockchainMessage(ChainSwitchMessage&& message) : type(MessageType::CHAIN_SWITCH_MESSAGE) {
	chainSwitchMessage = new ChainSwitchMessage(std::move(message));
}

BlockchainMessage::BlockchainMessage(const BlockchainMessage& other) : type(other.type) {
  switch (type) {
    case MessageType::NEW_BLOCK_MESSAGE:
      new (&newBlockMessage) NewBlockMessage(other.newBlockMessage);
      break;
    case MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE:
      new (&newAlternativeBlockMessage) NewAlternativeBlockMessage(other.newAlternativeBlockMessage);
      break;
    case MessageType::CHAIN_SWITCH_MESSAGE:
	  chainSwitchMessage = new ChainSwitchMessage(*other.chainSwitchMessage);
      break;
  }
}

BlockchainMessage::~BlockchainMessage() {
  switch (type) {
    case MessageType::NEW_BLOCK_MESSAGE:
      newBlockMessage.~NewBlockMessage();
      break;
    case MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE:
      newAlternativeBlockMessage.~NewAlternativeBlockMessage();
      break;
    case MessageType::CHAIN_SWITCH_MESSAGE:
	  delete chainSwitchMessage;
      break;
  }
}

BlockchainMessage::MessageType BlockchainMessage::getType() const {
  return type;
}

bool BlockchainMessage::getNewBlockHash(crypto::Hash& hash) const {
  if (type == MessageType::NEW_BLOCK_MESSAGE) {
    newBlockMessage.get(hash);
    return true;
  } else {
    return false;
  }
}

bool BlockchainMessage::getNewAlternativeBlockHash(crypto::Hash& hash) const {
  if (type == MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE) {
    newAlternativeBlockMessage.get(hash);
    return true;
  } else {
    return false;
  }
}

bool BlockchainMessage::getChainSwitch(std::vector<crypto::Hash>& hashes) const {
  if (type == MessageType::CHAIN_SWITCH_MESSAGE) {
    chainSwitchMessage->get(hashes);
    return true;
  } else {
    return false;
  }
}

}
