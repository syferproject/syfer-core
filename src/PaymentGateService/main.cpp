// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <memory>
#include <thread>

#include <string.h>

#include "PaymentGateService.h"
#include "version.h"

#ifdef _WIN32
#include <windows.h>
#include <winsvc.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#endif

#define SERVICE_NAME "Payment Gate"

PaymentGateService* ppg;

#ifdef _WIN32
SERVICE_STATUS_HANDLE serviceStatusHandle;

std::string GetLastErrorMessage(DWORD errorMessageID)
{
  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, errorMessageID, 0, (LPSTR)&messageBuffer, 0, NULL);

  std::string message(messageBuffer, size);

  LocalFree(messageBuffer);

  return message;
}

void __stdcall serviceHandler(DWORD fdwControl) {
  if (fdwControl == SERVICE_CONTROL_STOP) {
    logging::LoggerRef log(ppg->getLogger(), "serviceHandler");
    log(logging::INFO, logging::BRIGHT_YELLOW) << "Stop signal caught";

    SERVICE_STATUS serviceStatus{ SERVICE_WIN32_OWN_PROCESS, SERVICE_STOP_PENDING, 0, NO_ERROR, 0, 0, 0 };
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    ppg->stop();
  }
}

void __stdcall serviceMain(DWORD dwArgc, char **lpszArgv) {
  logging::LoggerRef logRef(ppg->getLogger(), "WindowsService");

  serviceStatusHandle = RegisterServiceCtrlHandler("PaymentGate", serviceHandler);
  if (serviceStatusHandle == NULL) {
    logRef(logging::FATAL, logging::BRIGHT_RED) << "Couldn't make RegisterServiceCtrlHandler call: " << GetLastErrorMessage(GetLastError());
    return;
  }

  SERVICE_STATUS serviceStatus{ SERVICE_WIN32_OWN_PROCESS, SERVICE_START_PENDING, 0, NO_ERROR, 0, 1, 3000 };
  if (SetServiceStatus(serviceStatusHandle, &serviceStatus) != TRUE) {
    logRef(logging::FATAL, logging::BRIGHT_RED) << "Couldn't make SetServiceStatus call: " << GetLastErrorMessage(GetLastError());
    return;
  }

  serviceStatus = { SERVICE_WIN32_OWN_PROCESS, SERVICE_RUNNING, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0 };
  if (SetServiceStatus(serviceStatusHandle, &serviceStatus) != TRUE) {
    logRef(logging::FATAL, logging::BRIGHT_RED) << "Couldn't make SetServiceStatus call: " << GetLastErrorMessage(GetLastError());
    return;
  }

  try {
    ppg->run();
  } catch (std::exception& ex) {
    logRef(logging::FATAL, logging::BRIGHT_RED) << "Error occurred: " << ex.what();
  }

  serviceStatus = { SERVICE_WIN32_OWN_PROCESS, SERVICE_STOPPED, 0, NO_ERROR, 0, 0, 0 };
  SetServiceStatus(serviceStatusHandle, &serviceStatus);
}
#else
int daemonize() {
  pid_t pid;
  pid = fork();

  if (pid < 0)
    return pid;

  if (pid > 0)
    return pid;

  if (setsid() < 0)
    return -1;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  pid = fork();

  if (pid < 0)
    return pid;

  if (pid > 0)
    return pid;

  umask(0);

  return 0;
}
#endif

int runDaemon() {
#ifdef _WIN32

  SERVICE_TABLE_ENTRY serviceTable[] {
    { "Payment Gate", serviceMain },
    { NULL, NULL }
  };

  logging::LoggerRef logRef(ppg->getLogger(), "RunService");

  if (StartServiceCtrlDispatcher(serviceTable) != TRUE) {
    logRef(logging::FATAL, logging::BRIGHT_RED) << "Couldn't start service: " << GetLastErrorMessage(GetLastError());
    return 1;
  }

  logRef(logging::INFO) << "Service stopped";
  return 0;

#else

  int daemonResult = daemonize();
  if (daemonResult > 0) {
    //parent
    return 0;
  } else if (daemonResult < 0) {
    //error occurred
    return 1;
  }

  ppg->run();

  return 0;

#endif
}

int registerService() {
#ifdef _WIN32
  logging::LoggerRef logRef(ppg->getLogger(), "ServiceRegistrator");

  char pathBuff[MAX_PATH];
  std::string modulePath;
  SC_HANDLE scManager = NULL;
  SC_HANDLE scService = NULL;
  int ret = 0;

  for (;;) {
    if (GetModuleFileName(NULL, pathBuff, ARRAYSIZE(pathBuff)) == 0) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "GetModuleFileName failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    modulePath.assign(pathBuff);

    std::string moduleDir = modulePath.substr(0, modulePath.find_last_of('\\') + 1);
    modulePath += " --config=" + moduleDir + "payment_service.conf -d";

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (scManager == NULL) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "OpenSCManager failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    scService = CreateService(scManager, SERVICE_NAME, NULL, SERVICE_QUERY_STATUS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
      SERVICE_ERROR_NORMAL, modulePath.c_str(), NULL, NULL, NULL, NULL, NULL);

    if (scService == NULL) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "CreateService failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    logRef(logging::INFO) << "Service is registered successfully";
    logRef(logging::INFO) << "Please make sure " << moduleDir + "payment_service.conf" << " exists";
    break;
  }

  if (scManager) {
    CloseServiceHandle(scManager);
  }

  if (scService) {
    CloseServiceHandle(scService);
  }

  return ret;
#else
  return 0;
#endif
}

int unregisterService() {
#ifdef _WIN32
  logging::LoggerRef logRef(ppg->getLogger(), "ServiceDeregistrator");

  SC_HANDLE scManager = NULL;
  SC_HANDLE scService = NULL;
  SERVICE_STATUS ssSvcStatus = { };
  int ret = 0;

  for (;;) {
    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scManager == NULL) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "OpenSCManager failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    scService = OpenService(scManager, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (scService == NULL) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "OpenService failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    if (ControlService(scService, SERVICE_CONTROL_STOP, &ssSvcStatus)) {
      logRef(logging::INFO) << "Stopping " << SERVICE_NAME;
      Sleep(1000);

      while (QueryServiceStatus(scService, &ssSvcStatus)) {
        if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
          logRef(logging::INFO) << "Waiting...";
          Sleep(1000);
        } else {
          break;
        }
      }

      std::cout << std::endl;
      if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) {
        logRef(logging::INFO) << SERVICE_NAME << " is stopped";
      } else {
        logRef(logging::FATAL, logging::BRIGHT_RED) << SERVICE_NAME << " failed to stop" << std::endl;
      }
    }

    if (!DeleteService(scService)) {
      logRef(logging::FATAL, logging::BRIGHT_RED) << "DeleteService failed with error: " << GetLastErrorMessage(GetLastError());
      ret = 1;
      break;
    }

    logRef(logging::INFO) << SERVICE_NAME << " is removed";
    break;
  }

  if (scManager) {
    CloseServiceHandle(scManager);
  }

  if (scService) {
    CloseServiceHandle(scService);
  }

  return ret;
#else
  return 0;
#endif
}

int main(int argc, char** argv) {
  PaymentGateService pg; 
  ppg = &pg;

  try {
    if (!pg.init(argc, argv)) {
      return 0; //help message requested or so
    }

    logging::LoggerRef(pg.getLogger(), "main")(logging::INFO) << SYFR_PAYMENT_SERVICE_RELEASE_VERSION;

    const auto& config = pg.getConfig();

    if (config.gateConfiguration.generateNewContainer) {
      platform_system::Dispatcher d;
      generateNewWallet(pg.getCurrency(), pg.getWalletConfig(), pg.getLogger(), d);
      return 0;
    }

    if (config.gateConfiguration.registerService) {
      return registerService();
    }

    if (config.gateConfiguration.unregisterService) {
      return unregisterService();
    }

    if (config.gateConfiguration.daemonize) {
      if (runDaemon() != 0) {
        throw std::runtime_error("Failed to start daemon");
      }
    } else {
      pg.run();
    }

  } catch (payment_service::ConfigurationError& ex) {
    std::cerr << "Configuration error: " << ex.what() << std::endl;
    return 1;
  } catch (std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
