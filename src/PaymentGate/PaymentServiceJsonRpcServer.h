// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>

#include "Common/JsonValue.h"
#include "JsonRpcServer/JsonRpcServer.h"
#include "PaymentServiceJsonRpcMessages.h"
#include "Serialization/JsonInputValueSerializer.h"
#include "Serialization/JsonOutputStreamSerializer.h"

namespace payment_service {

class WalletService;

class PaymentServiceJsonRpcServer : public cn::JsonRpcServer {
public:
  PaymentServiceJsonRpcServer(platform_system::Dispatcher& sys, platform_system::Event& stopEvent, WalletService& service, logging::ILogger& loggerGroup);
  PaymentServiceJsonRpcServer(const PaymentServiceJsonRpcServer&) = delete;
  virtual ~PaymentServiceJsonRpcServer() = default;

protected:
  void processJsonRpcRequest(const common::JsonValue& req, common::JsonValue& resp) override;

private:
  WalletService& service;
  logging::LoggerRef logger;

  using HandlerFunction = std::function<void(const common::JsonValue &jsonRpcParams, common::JsonValue &jsonResponse)>;

  template <typename RequestType, typename ResponseType, typename RequestHandler>
  HandlerFunction jsonHandler(RequestHandler handler) const {
    return [handler] (const common::JsonValue& jsonRpcParams, common::JsonValue& jsonResponse) mutable {
      RequestType request;
      ResponseType response;

      try {
        cn::JsonInputValueSerializer inputSerializer(jsonRpcParams);
        serialize(request, inputSerializer);
      } catch (std::exception&) {
        makeGenericErrorReponse(jsonResponse, "Invalid Request", -32600);
        return;
      }

      std::error_code ec = handler(request, response);
      if (ec) {
        makeErrorResponse(ec, jsonResponse);
        return;
      }

      cn::JsonOutputStreamSerializer outputSerializer;
      serialize(response, outputSerializer);
      fillJsonResponse(outputSerializer.getValue(), jsonResponse);
    };
  }

  std::unordered_map<std::string, HandlerFunction> handlers;

  std::error_code handleReset(const Reset::Request &request, const Reset::Response &response);
  std::error_code handleSave(const Save::Request& request, const Save::Response& response);
  std::error_code handleExportWallet(const ExportWallet::Request &request, const ExportWallet::Response &response);
  std::error_code handleExportWalletKeys(const ExportWalletKeys::Request &request, const ExportWalletKeys::Response &response);
  std::error_code handleCreateIntegrated(const CreateIntegrated::Request& request, CreateIntegrated::Response& response);
  std::error_code handleSplitIntegrated(const SplitIntegrated::Request& request, SplitIntegrated::Response& response);
  std::error_code handleCreateAddress(const CreateAddress::Request& request, CreateAddress::Response& response);
  std::error_code handleCreateAddressList(const CreateAddressList::Request& request, CreateAddressList::Response& response);
  std::error_code handleDeleteAddress(const DeleteAddress::Request& request, const DeleteAddress::Response& response);
  std::error_code handleGetSpendKeys(const GetSpendKeys::Request& request, GetSpendKeys::Response& response);
  std::error_code handleGetBalance(const GetBalance::Request& request, GetBalance::Response& response);
  std::error_code handleGetBlockHashes(const GetBlockHashes::Request& request, GetBlockHashes::Response& response);
  std::error_code handleGetTransactionHashes(const GetTransactionHashes::Request& request, GetTransactionHashes::Response& response);
  std::error_code handleGetTransactions(const GetTransactions::Request& request, GetTransactions::Response& response);
  std::error_code handleGetUnconfirmedTransactionHashes(const GetUnconfirmedTransactionHashes::Request& request, GetUnconfirmedTransactionHashes::Response& response);
  std::error_code handleGetTransaction(const GetTransaction::Request& request, GetTransaction::Response& response);
  std::error_code handleSendTransaction(const SendTransaction::Request& request, SendTransaction::Response& response);
  std::error_code handleCreateDelayedTransaction(const CreateDelayedTransaction::Request& request, CreateDelayedTransaction::Response& response);
  std::error_code handleGetDelayedTransactionHashes(const GetDelayedTransactionHashes::Request& request, GetDelayedTransactionHashes::Response& response);
  std::error_code handleDeleteDelayedTransaction(const DeleteDelayedTransaction::Request& request, const DeleteDelayedTransaction::Response& response);
  std::error_code handleSendDelayedTransaction(const SendDelayedTransaction::Request& request, const SendDelayedTransaction::Response& response);
  std::error_code handleGetViewKey(const GetViewKey::Request& request, GetViewKey::Response& response);
  std::error_code handleGetStatus(const GetStatus::Request& request, GetStatus::Response& response);
  std::error_code handleCreateDeposit(const CreateDeposit::Request& request, CreateDeposit::Response& response);
  std::error_code handleSendDeposit(const SendDeposit::Request& request, SendDeposit::Response& response);
  std::error_code handleWithdrawDeposit(const WithdrawDeposit::Request &request, WithdrawDeposit::Response &response);
  std::error_code handleGetDeposit(const GetDeposit::Request& request, GetDeposit::Response& response);
  std::error_code handleGetAddresses(const GetAddresses::Request& request, GetAddresses::Response& response);
  std::error_code handleGetMessagesFromExtra(const GetMessagesFromExtra::Request& request, GetMessagesFromExtra::Response& response);
  std::error_code handleEstimateFusion(const EstimateFusion::Request& request, EstimateFusion::Response& response);
  std::error_code handleSendFusionTransaction(const SendFusionTransaction::Request& request, SendFusionTransaction::Response& response);
};

}//namespace payment_service
