// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ConsoleLogger.h"
#include <iostream>
#include <unordered_map>
#include <Common/ConsoleTools.h>


namespace logging {

using common::console::Color;

ConsoleLogger::ConsoleLogger(Level level) : CommonLogger(level) {
}

void ConsoleLogger::doLogString(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex);
  bool readingText = true;
  bool changedColor = false;
  std::string color = "";

  static std::unordered_map<std::string, Color> colorMapping = {
    { BLUE, Color::Blue },
    { GREEN, Color::Green },
    { RED, Color::Red },
    { YELLOW, Color::Yellow },
    { WHITE, Color::White },
    { CYAN, Color::Cyan },
    { MAGENTA, Color::Magenta },

    { BRIGHT_BLUE, Color::BrightBlue },
    { BRIGHT_GREEN, Color::BrightGreen },
    { BRIGHT_RED, Color::BrightRed },
    { BRIGHT_YELLOW, Color::BrightYellow },
    { BRIGHT_WHITE, Color::BrightWhite },
    { BRIGHT_CYAN, Color::BrightCyan },
    { BRIGHT_MAGENTA, Color::BrightMagenta },

    { DEFAULT, Color::Default }
  };

  for (size_t charPos = 0; charPos < message.size(); ++charPos) {
    if (message[charPos] == ILogger::COLOR_DELIMETER) {
      readingText = !readingText;
      color += message[charPos];
      if (readingText) {
        auto it = colorMapping.find(color);
        common::console::setTextColor(it == colorMapping.end() ? Color::Default : it->second);
        changedColor = true;
        color.clear();
      }
    } else if (readingText) {
      std::cout << message[charPos];
    } else {
      color += message[charPos];
    }
  }

  if (changedColor) {
    common::console::setTextColor(Color::Default);
  }
}

}
