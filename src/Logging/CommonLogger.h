// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <set>
#include "ILogger.h"

namespace logging {

class CommonLogger : public ILogger {
public:

  virtual void operator()(const std::string& category, Level level, boost::posix_time::ptime time, const std::string& body) override;
  virtual void enableCategory(const std::string& category);
  virtual void disableCategory(const std::string& category);
  virtual void setMaxLevel(Level level);

  void setPattern(const std::string& pattern);

protected:
  std::set<std::string> disabledCategories;
  Level logLevel;
  std::string pattern;

  CommonLogger(Level level);
  virtual void doLogString(const std::string& message);
};

}
