// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "HttpResponse.h"

#include <stdexcept>

namespace {

  const char *getStatusString(cn::HttpResponse::HTTP_STATUS status)
  {
    switch (status)
    {
    case cn::HttpResponse::STATUS_200:
      return "200 OK";
    case cn::HttpResponse::STATUS_401:
      return "401 Unauthorized";
    case cn::HttpResponse::STATUS_404:
      return "404 Not Found";
    case cn::HttpResponse::STATUS_500:
      return "500 Internal Server Error";
    default:
      throw std::runtime_error("Unknown HTTP status code is given");
    }
  }

  const char *getErrorBody(cn::HttpResponse::HTTP_STATUS status)
  {
    switch (status)
    {
    case cn::HttpResponse::STATUS_401:
      return "Authorization required\n";
    case cn::HttpResponse::STATUS_404:
      return "Requested url is not found\n";
    case cn::HttpResponse::STATUS_500:
      return "Internal server error is occurred\n";
    default:
      throw std::runtime_error("Error body for given status is not available");
    }
  }

} //namespace

namespace cn {

HttpResponse::HttpResponse() {
  status = STATUS_200;
  headers["Server"] = "cn-based HTTP server";
}

void HttpResponse::setStatus(HTTP_STATUS s) {
  status = s;

  if (status != HttpResponse::STATUS_200) {
    setBody(getErrorBody(status));
  }
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
  headers[name] = value;
}

void HttpResponse::setBody(const std::string& b) {
  body = b;
  if (!body.empty()) {
    headers["Content-Length"] = std::to_string(body.size());
  } else {
    headers.erase("Content-Length");
  }
}

std::ostream& HttpResponse::printHttpResponse(std::ostream& os) const {
  os << "HTTP/1.1 " << getStatusString(status) << "\r\n";

  for (auto pair: headers) {
    os << pair.first << ": " << pair.second << "\r\n";
  }
  os << "\r\n";

  if (!body.empty()) {
    os << body;
  }

  return os;
}

} //namespace cn
