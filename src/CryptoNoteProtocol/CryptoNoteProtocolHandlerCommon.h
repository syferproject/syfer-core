// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace cn
{
  struct NOTIFY_NEW_BLOCK_request;
  struct NOTIFY_NEW_TRANSACTIONS_request;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_cryptonote_protocol {
    virtual void relay_block(NOTIFY_NEW_BLOCK_request& arg) = 0;
    virtual void relay_transactions(NOTIFY_NEW_TRANSACTIONS_request& arg) = 0;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct cryptonote_protocol_stub: public i_cryptonote_protocol {
    virtual void relay_block(NOTIFY_NEW_BLOCK_request& arg) override {}
    virtual void relay_transactions(NOTIFY_NEW_TRANSACTIONS_request& arg) override {}
  };
}
