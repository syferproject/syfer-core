// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "Common/ConsoleHandler.h"

#include <Logging/LoggerRef.h>
#include <Logging/LoggerManager.h>

namespace cn {
class core;
class Currency;
class NodeServer;
}

class DaemonCommandsHandler
{
public:
  DaemonCommandsHandler(cn::core& core, cn::NodeServer& srv, logging::LoggerManager& log);

  bool start_handling() {
    m_consoleHandler.start();
    return true;
  }

  void stop_handling() {
    m_consoleHandler.stop();
  }

private:

  common::ConsoleHandler m_consoleHandler;
  cn::core& m_core;
  cn::NodeServer& m_srv;
  logging::LoggerRef logger;
  logging::LoggerManager& m_logManager;

  const std::string get_commands_str();
  bool print_block_by_height(uint32_t height);
  bool print_block_by_hash(const std::string& arg);
  uint64_t calculatePercent(const cn::Currency& currency, uint64_t value, uint64_t total);

  bool exit(const std::vector<std::string>& args);
  bool help(const std::vector<std::string>& args);
  bool print_pl(const std::vector<std::string>& args);
  bool show_hr(const std::vector<std::string>& args);
  bool hide_hr(const std::vector<std::string>& args);
  bool rollbackchainto(uint32_t height);  
  bool rollback_chain(const std::vector<std::string>& args);  
  bool print_cn(const std::vector<std::string>& args);
  bool print_bc(const std::vector<std::string>& args);
  bool print_bci(const std::vector<std::string>& args);
  bool set_log(const std::vector<std::string>& args);
  bool print_block(const std::vector<std::string>& args);
  bool print_tx(const std::vector<std::string>& args);
  bool print_pool(const std::vector<std::string>& args);
  bool print_pool_sh(const std::vector<std::string>& args);
  bool print_stat(const std::vector<std::string>& args);
  bool save(const std::vector<std::string> &args);

  bool start_mining(const std::vector<std::string>& args);
  bool stop_mining(const std::vector<std::string>& args);
};
