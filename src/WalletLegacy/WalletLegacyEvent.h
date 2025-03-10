// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "IWalletLegacy.h"
#include "Common/ObserverManager.h"

namespace cn
{

class WalletLegacyEvent
{
public:
  virtual ~WalletLegacyEvent() {
  };

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) = 0;
};

class WalletTransactionUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletTransactionUpdatedEvent(TransactionId transactionId) : m_id(transactionId) {};
  virtual ~WalletTransactionUpdatedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::transactionUpdated, m_id);
  }

private:
  TransactionId m_id;
};

class WalletSendTransactionCompletedEvent : public WalletLegacyEvent
{
public:
  WalletSendTransactionCompletedEvent(TransactionId transactionId, std::error_code result) : m_id(transactionId), m_error(result) {};
  virtual ~WalletSendTransactionCompletedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::sendTransactionCompleted, m_id, m_error);
  }

private:
  TransactionId m_id;
  std::error_code m_error;
};

class WalletExternalTransactionCreatedEvent : public WalletLegacyEvent
{
public:
  WalletExternalTransactionCreatedEvent(TransactionId transactionId) : m_id(transactionId) {};
  virtual ~WalletExternalTransactionCreatedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::externalTransactionCreated, m_id);
  }
private:
  TransactionId m_id;
};

class WalletDepositUpdatedEvent : public WalletLegacyEvent {
public:
  WalletDepositUpdatedEvent(DepositId& depositId) : updatedDeposit(depositId) {}

  virtual ~WalletDepositUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override {
    observer.notify(&IWalletLegacyObserver::depositUpdated, updatedDeposit);
  }
private:
  DepositId updatedDeposit;
};

class WalletDepositsUpdatedEvent : public WalletLegacyEvent {
public:
  WalletDepositsUpdatedEvent(std::vector<DepositId>&& depositIds) : updatedDeposits(depositIds) {}

  virtual ~WalletDepositsUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override {
    observer.notify(&IWalletLegacyObserver::depositsUpdated, updatedDeposits);
  }
private:
  std::vector<DepositId> updatedDeposits;
};

class WalletSynchronizationProgressUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletSynchronizationProgressUpdatedEvent(uint32_t current, uint32_t total) : m_current(current), m_total(total) {};
  virtual ~WalletSynchronizationProgressUpdatedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::synchronizationProgressUpdated, m_current, m_total);
  }

private:
  uint32_t m_current;
  uint32_t m_total;
};

class WalletSynchronizationCompletedEvent : public WalletLegacyEvent {
public:
  WalletSynchronizationCompletedEvent(uint32_t current, uint32_t total, std::error_code result) : m_ec(result) {};
  virtual ~WalletSynchronizationCompletedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override {
    observer.notify(&IWalletLegacyObserver::synchronizationCompleted, m_ec);
  }

private:
  std::error_code m_ec;
};

class WalletActualBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {};
  virtual ~WalletActualBalanceUpdatedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::actualBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {};
  virtual ~WalletPendingBalanceUpdatedEvent() {};

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer) override
  {
    observer.notify(&IWalletLegacyObserver::pendingBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletActualDepositBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualDepositBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletActualDepositBalanceUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::actualDepositBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingDepositBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingDepositBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletPendingDepositBalanceUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::pendingDepositBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

/* investments */

class WalletActualInvestmentBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletActualInvestmentBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletActualInvestmentBalanceUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::actualInvestmentBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};

class WalletPendingInvestmentBalanceUpdatedEvent : public WalletLegacyEvent
{
public:
  WalletPendingInvestmentBalanceUpdatedEvent(uint64_t balance) : m_balance(balance) {}
  virtual ~WalletPendingInvestmentBalanceUpdatedEvent() {}

  virtual void notify(tools::ObserverManager<cn::IWalletLegacyObserver>& observer)
  {
    observer.notify(&IWalletLegacyObserver::pendingInvestmentBalanceUpdated, m_balance);
  }
private:
  uint64_t m_balance;
};





} /* namespace cn */
