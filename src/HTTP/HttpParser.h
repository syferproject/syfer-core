// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HTTPPARSER_H_
#define HTTPPARSER_H_

#include <iostream>
#include <map>
#include <string>
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace cn {

//Blocking HttpParser
class HttpParser {
public:
  HttpParser() {};

  void receiveRequest(std::istream& stream, HttpRequest& request);
  void receiveResponse(std::istream& stream, HttpResponse& response);
  static HttpResponse::HTTP_STATUS parseResponseStatusFromString(const std::string& status);
private:
  void readWord(std::istream& stream, std::string& word);
  void readHeaders(std::istream& stream, HttpRequest::Headers &headers);
  bool readHeader(std::istream& stream, std::string& name, std::string& value);
  size_t getBodyLen(const HttpRequest::Headers& headers);
  void readBody(std::istream& stream, std::string& body, const size_t bodyLen);
};

} //namespace cn

#endif /* HTTPPARSER_H_ */
