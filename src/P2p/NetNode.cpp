// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2020 The Karbo developers
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "NetNode.h"

#include <algorithm>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/format.hpp>

#include <miniupnpc/include/miniupnpc.h>
#include <miniupnpc/include/upnpcommands.h>

#include <System/Context.h>
#include <System/ContextGroupTimeout.h>
#include <System/EventLock.h>
#include <System/InterruptedException.h>
#include <System/Ipv4Address.h>
#include <System/Ipv4Resolver.h>
#include <System/TcpListener.h>
#include <System/TcpConnector.h>

#include "version.h"
#include "Common/StdInputStream.h"
#include "Common/StdOutputStream.h"
#include "Common/Util.h"
#include "crypto/crypto.h"

#include <CryptoNoteConfig.h>
#include "ConnectionContext.h"
#include "LevinProtocol.h"
#include "P2pProtocolDefinitions.h"

#include "Serialization/BinaryInputStreamSerializer.h"
#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "Common/StringTools.h"

using namespace common;
using namespace logging;
using namespace cn;

namespace {

size_t get_random_index_with_fixed_probability(size_t max_index) {
  //divide by zero workaround
  if (!max_index) {
    return 0;
  }

  size_t x = crypto::rand<size_t>() % (max_index + 1);
  return (x * x * x ) / (max_index * max_index); //parabola \/
}

void addPortMapping(const logging::LoggerRef& logger, uint32_t port) {
  // Add UPnP port mapping
  logger(INFO) <<  "Attempting to add IGD port mapping.";
  int result;
  UPNPDev *deviceList = upnpDiscover(1000, nullptr, nullptr, 0, 0, 2, &result);
  UPNPUrls urls;
  IGDdatas igdData;
  char lanAddress[64];
  result = UPNP_GetValidIGD(deviceList, &urls, &igdData, lanAddress, sizeof lanAddress);
  freeUPNPDevlist(deviceList);
  if (result != 0) {
    if (result == 1) {
      std::ostringstream portString;
      portString << port;
      if (UPNP_AddPortMapping(urls.controlURL, igdData.first.servicetype, portString.str().c_str(),
        portString.str().c_str(), lanAddress, "syfer", "TCP", nullptr, "0") != 0) {
        logger(ERROR) << "UPNP port mapping failed.";
      } else {
        logger(INFO, BRIGHT_GREEN) << "Added IGD port mapping.";
      }
    } else if (result == 2) {
      logger(INFO) <<  "IGD was found but reported as not connected.";
    } else if (result == 3) {
      logger(INFO) <<  "UPnP device was found but not recognized as IGD.";
    } else {
      logger(ERROR) << "UPNP_GetValidIGD returned an unknown result code.";
    }

    FreeUPNPUrls(&urls);
  } else {
    logger(INFO) <<  "No IGD was found.";
  }
}

bool parse_peer_from_string(NetworkAddress& pe, const std::string& node_addr) {
  return common::parseIpAddressAndPort(pe.ip, pe.port, node_addr);
}

}


namespace cn
{
  namespace
  {
    const command_line::arg_descriptor<std::string> arg_p2p_bind_ip        = {"p2p-bind-ip", "Interface for p2p network protocol", "0.0.0.0"};
    const command_line::arg_descriptor<std::string> arg_p2p_bind_port      = {"p2p-bind-port", "Port for p2p network protocol", std::to_string(cn::P2P_DEFAULT_PORT)};
    const command_line::arg_descriptor<uint32_t>    arg_p2p_external_port  = {"p2p-external-port", "External port for p2p network protocol (if port forwarding used with NAT)", 0};
    const command_line::arg_descriptor<bool>        arg_p2p_allow_local_ip = {"allow-local-ip", "Allow local ip add to peer list, mostly in debug purposes"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_peer   = {"add-peer", "Manually add peer to local peerlist"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_priority_node   = {"add-priority-node", "Specify list of peers to connect to and attempt to keep the connection open"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_exclusive_node   = {"add-exclusive-node", "Specify list of peers to connect to only."
                                                                                                  " If this option is given the options add-priority-node and seed-node are ignored"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_seed_node   = {"seed-node", "Connect to a node to retrieve peer addresses, and disconnect"};
    const command_line::arg_descriptor<bool> arg_p2p_hide_my_port   =    {"hide-my-port", "Do not announce yourself as peerlist candidate", false, true};

    std::string print_peerlist_to_string(const std::list<PeerlistEntry> &pl)
    {
      time_t now_time = 0;
      time(&now_time);
      std::stringstream ss;
      ss << std::left
         << std::setw(20) << "Peer id"
         << std::setw(25) << "Remote host"
         << std::setw(16) << "Last seen" << std::endl;
      for (const auto &pe : pl)
      {
        ss << std::setw(20) << std::hex << pe.id
           << std::setw(25) << common::ipAddressToString(pe.adr.ip) + ":" + std::to_string(pe.adr.port)
           << std::setw(16) << common::timeIntervalToString(now_time - pe.last_seen) << std::endl;
      }
      return ss.str();
    }
  }

  //-----------------------------------------------------------------------------------
  // P2pConnectionContext implementation
  //-----------------------------------------------------------------------------------

  bool P2pConnectionContext::pushMessage(P2pMessage&& msg) {
    writeQueueSize += msg.size();

    if (writeQueueSize > P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE) {
      logger(DEBUGGING) << *this << "P2pConnectionContext::pushMessage() write queue overflows. Interrupt connection";
      interrupt();
      return false;
    }

    writeQueue.push_back(std::move(msg));
    queueEvent.set();
    return true;
  }

  std::vector<P2pMessage> P2pConnectionContext::popBuffer() {
    writeOperationStartTime = TimePoint();

    while (writeQueue.empty() && !stopped) {
      queueEvent.wait();
    }

    std::vector<P2pMessage> msgs(std::move(writeQueue));
    writeQueue.clear();
    writeQueueSize = 0;
    writeOperationStartTime = Clock::now();
    queueEvent.clear();
    return msgs;
  }

  uint64_t P2pConnectionContext::writeDuration(TimePoint now) const { // in milliseconds
    return writeOperationStartTime == TimePoint() ? 0 : std::chrono::duration_cast<std::chrono::milliseconds>(now - writeOperationStartTime).count();
  }

  void P2pConnectionContext::interrupt() {
    logger(DEBUGGING) << *this << "P2pConnectionContext::interrupt()";
    assert(context != nullptr);
    stopped = true;
    queueEvent.set();
    context->interrupt();
  }

  template <typename Command, typename Handler>
  int invokeAdaptor(const BinaryArray& reqBuf, BinaryArray& resBuf, P2pConnectionContext& ctx, Handler handler) {
    using Request = typename Command::request;
    using Response = typename Command::response;
    int command = Command::ID;

    Request req = boost::value_initialized<Request>();

    if (!LevinProtocol::decode(reqBuf, req)) {
      throw std::runtime_error("Failed to load_from_binary in command " + std::to_string(command));
    }

    Response res = boost::value_initialized<Response>();
    int ret = handler(command, req, res, ctx);
    resBuf = LevinProtocol::encode(res);
    return ret;
  }

  NodeServer::NodeServer(platform_system::Dispatcher& dispatcher, cn::CryptoNoteProtocolHandler& payload_handler, logging::ILogger& log) :
    m_dispatcher(dispatcher),
    m_workingContextGroup(dispatcher),
    m_stopEvent(m_dispatcher),
    m_idleTimer(m_dispatcher),
    m_timeoutTimer(m_dispatcher),
    logger(log, "node_server"),
    m_payload_handler(payload_handler),
    m_timedSyncTimer(m_dispatcher)    
  {
  }

  void NodeServer::serialize(ISerializer& s) {
    uint8_t version = 1;
    s(version, "version");

    if (version != 1) {
      return;
    }

    s(m_peerlist, "peerlist");
    s(m_config.m_peer_id, "peer_id");
  }

#define INVOKE_HANDLER(CMD, Handler)                                                                                                             \
  case CMD::ID:                                                                                                                                  \
  {                                                                                                                                              \
    ret = invokeAdaptor<CMD>(cmd.buf, out, ctx, boost::bind(Handler, this, boost::arg<1>(), boost::arg<2>(), boost::arg<3>(), boost::arg<4>())); \
    break;                                                                                                                                       \
  }

  int NodeServer::handleCommand(const LevinProtocol::Command &cmd, BinaryArray &out, P2pConnectionContext &ctx, bool &handled)
  {
    int ret = 0;
    handled = true;

    if (cmd.isResponse && cmd.command == COMMAND_TIMED_SYNC::ID)
    {
      if (!handleTimedSyncResponse(cmd.buf, ctx))
      {
        // invalid response, close connection
        ctx.m_state = CryptoNoteConnectionContext::state_shutdown;
      }
      return 0;
    }

    switch (cmd.command)
    {
      INVOKE_HANDLER(COMMAND_HANDSHAKE, &NodeServer::handle_handshake)
      INVOKE_HANDLER(COMMAND_TIMED_SYNC, &NodeServer::handle_timed_sync)
      INVOKE_HANDLER(COMMAND_PING, &NodeServer::handle_ping)
#ifdef ALLOW_DEBUG_COMMANDS
      INVOKE_HANDLER(COMMAND_REQUEST_STAT_INFO, &NodeServer::handle_get_stat_info)
      INVOKE_HANDLER(COMMAND_REQUEST_NETWORK_STATE, &NodeServer::handle_get_network_state)
      INVOKE_HANDLER(COMMAND_REQUEST_PEER_ID, &NodeServer::handle_get_peer_id)
#endif
    default:
    {
      handled = false;
      ret = m_payload_handler.handleCommand(cmd.isNotify, cmd.command, cmd.buf, out, ctx, handled);
    }
    }

    return ret;
  }

#undef INVOKE_HANDLER

  //-----------------------------------------------------------------------------------

  void NodeServer::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_p2p_bind_ip);
    command_line::add_arg(desc, arg_p2p_bind_port);
    command_line::add_arg(desc, arg_p2p_external_port);
    command_line::add_arg(desc, arg_p2p_allow_local_ip);
    command_line::add_arg(desc, arg_p2p_add_peer);
    command_line::add_arg(desc, arg_p2p_add_priority_node);
    command_line::add_arg(desc, arg_p2p_add_exclusive_node);
    command_line::add_arg(desc, arg_p2p_seed_node);
    command_line::add_arg(desc, arg_p2p_hide_my_port);
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::init_config() {
    try {
      std::string state_file_path = m_config_folder + "/" + m_p2p_state_filename;
      bool loaded = false;

      try {
        std::ifstream p2p_data;
        p2p_data.open(state_file_path, std::ios_base::binary | std::ios_base::in);

        if (!p2p_data.fail()) {
          StdInputStream inputStream(p2p_data);
          BinaryInputStreamSerializer a(inputStream);
          cn::serialize(*this, a);
          loaded = true;
        }
      } catch (const std::exception&) {
      }

      if (!loaded) {
        make_default_config();
      }

      //at this moment we have hardcoded config
      m_config.m_net_config.handshake_interval = cn::P2P_DEFAULT_HANDSHAKE_INTERVAL;
      m_config.m_net_config.connections_count = cn::P2P_DEFAULT_CONNECTIONS_COUNT;
      m_config.m_net_config.packet_max_size = cn::P2P_DEFAULT_PACKET_MAX_SIZE; //20 MB limit
      m_config.m_net_config.config_id = 0; // initial config
      m_config.m_net_config.connection_timeout = cn::P2P_DEFAULT_CONNECTION_TIMEOUT;
      m_config.m_net_config.ping_connection_timeout = cn::P2P_DEFAULT_PING_CONNECTION_TIMEOUT;
      m_config.m_net_config.send_peerlist_sz = cn::P2P_DEFAULT_PEERS_IN_HANDSHAKE;

      m_first_connection_maker_call = true;
    } catch (const std::exception& e) {
      logger(ERROR) << "init_config failed: " << e.what();
      return false;
    }
    return true;
  }

  //-----------------------------------------------------------------------------------
  void NodeServer::for_each_connection(const std::function<void(CryptoNoteConnectionContext &, PeerIdType)> &f)
  {
    for (auto& ctx : m_connections) {
      f(ctx.second, ctx.second.peerId);
    }
  }

  //-----------------------------------------------------------------------------------
  void NodeServer::externalRelayNotifyToAll(int command, const BinaryArray &data_buff, const net_connection_id *excludeConnection)
  {
    m_dispatcher.remoteSpawn([this, command, data_buff, excludeConnection] {
      relay_notify_to_all(command, data_buff, excludeConnection);
    });
  }

  //-----------------------------------------------------------------------------------
  void NodeServer::externalRelayNotifyToList(int command, const BinaryArray &data_buff, const std::list<boost::uuids::uuid> &relayList)
  {
    m_dispatcher.remoteSpawn([this, command, data_buff, relayList] {
      forEachConnection([&relayList, &command, &data_buff](P2pConnectionContext &conn) {
        if (std::find(relayList.begin(), relayList.end(), conn.m_connection_id) != relayList.end())
        {
          if (conn.peerId && (conn.m_state == CryptoNoteConnectionContext::state_normal || conn.m_state == CryptoNoteConnectionContext::state_synchronizing))
          {
            conn.pushMessage(P2pMessage(P2pMessage::NOTIFY, command, data_buff));
          }
        }
      });
    });
  }


  void NodeServer::drop_connection(CryptoNoteConnectionContext &context, bool add_fail)
  {
    context.m_state = CryptoNoteConnectionContext::state_shutdown;
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::make_default_config()
  {
    m_config.m_peer_id  = crypto::rand<uint64_t>();
    return true;
  }

  bool NodeServer::is_addr_recently_failed(const uint32_t address_ip)
  {
    std::unique_lock<std::mutex> lock(mutex);
    auto i = m_host_fails_score.find(address_ip);
    if (i != m_host_fails_score.end())
      return true;

    return false;
  }

  bool NodeServer::is_peer_used(const AnchorPeerlistEntry &peer) const
  {
    if (m_config.m_peer_id == peer.id)
      return true; //dont make connections to ourself

    for (const auto &kv : m_connections)
    {
      const auto &cntxt = kv.second;
      if (cntxt.peerId == peer.id || (!cntxt.m_is_income && peer.adr.ip == cntxt.m_remote_ip && peer.adr.port == cntxt.m_remote_port))
      {
        return true;
      }
    }
    return false;
  }

  //-----------------------------------------------------------------------------------

  bool NodeServer::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_p2p_bind_ip);
    m_port = command_line::get_arg(vm, arg_p2p_bind_port);
    m_external_port = command_line::get_arg(vm, arg_p2p_external_port);
    m_allow_local_ip = command_line::get_arg(vm, arg_p2p_allow_local_ip);

    if (command_line::has_arg(vm, arg_p2p_add_peer))
    {
      std::vector<std::string> perrs = command_line::get_arg(vm, arg_p2p_add_peer);
      for(const std::string& pr_str: perrs)
      {
        PeerlistEntry pe = boost::value_initialized<PeerlistEntry>();
        pe.id = crypto::rand<uint64_t>();
        bool r = parse_peer_from_string(pe.adr, pr_str);
        if (!r) { logger(ERROR, BRIGHT_RED) << "Failed to parse address from string: " << pr_str; return false; }
        m_command_line_peers.push_back(pe);
      }
    }

    if (command_line::has_arg(vm,arg_p2p_add_exclusive_node)) {
      if (!parse_peers_and_add_to_container(vm, arg_p2p_add_exclusive_node, m_exclusive_peers))
        return false;
    }
    if (command_line::has_arg(vm, arg_p2p_add_priority_node)) {
      if (!parse_peers_and_add_to_container(vm, arg_p2p_add_priority_node, m_priority_peers))
        return false;
    }
    if (command_line::has_arg(vm, arg_p2p_seed_node)) {
      if (!parse_peers_and_add_to_container(vm, arg_p2p_seed_node, m_seed_nodes))
        return false;
    }

    if (command_line::has_arg(vm, arg_p2p_hide_my_port)) {
      m_hide_my_port = true;
    }

    return true;
  }

  bool NodeServer::handleConfig(const NetNodeConfig& config) {
    m_bind_ip = config.getBindIp();
    m_port = std::to_string(config.getBindPort());
    m_external_port = config.getExternalPort();
    m_allow_local_ip = config.getAllowLocalIp();

    auto peers = config.getPeers();
    std::copy(peers.begin(), peers.end(), std::back_inserter(m_command_line_peers));

    auto exclusiveNodes = config.getExclusiveNodes();
    std::copy(exclusiveNodes.begin(), exclusiveNodes.end(), std::back_inserter(m_exclusive_peers));

    auto priorityNodes = config.getPriorityNodes();
    std::copy(priorityNodes.begin(), priorityNodes.end(), std::back_inserter(m_priority_peers));

    auto seedNodes = config.getSeedNodes();
    std::copy(seedNodes.begin(), seedNodes.end(), std::back_inserter(m_seed_nodes));

    m_hide_my_port = config.getHideMyPort();
    return true;
  }

  bool NodeServer::append_net_address(std::vector<NetworkAddress>& nodes, const std::string& addr) {
    size_t pos = addr.find_last_of(':');
    if (!(std::string::npos != pos && addr.length() - 1 != pos && 0 != pos)) {
      logger(ERROR, BRIGHT_RED) << "Failed to parse seed address from string: '" << addr << '\'';
      return false;
    }

    std::string host = addr.substr(0, pos);

    try {
      uint32_t port = common::fromString<uint32_t>(addr.substr(pos + 1));

      platform_system::Ipv4Resolver resolver(m_dispatcher);
      auto address = resolver.resolve(host);
      nodes.push_back(NetworkAddress{hostToNetwork(address.getValue()), port});

      logger(TRACE) << "Added seed node: " << nodes.back() << " (" << host << ")";

    } catch (const std::exception& e) {
      logger(ERROR, BRIGHT_YELLOW) << "Failed to resolve host name '" << host << "': " << e.what();
      return false;
    }

    return true;
  }


  //-----------------------------------------------------------------------------------

  bool NodeServer::init(const NetNodeConfig& config) {
    if (!config.getTestnet())
    {
      for (auto seed : cn::SEED_NODES)
      {
        append_net_address(m_seed_nodes, seed);
      }
    }
    else
    {
      for (auto seed : cn::TESTNET_SEED_NODES)
      {
        append_net_address(m_seed_nodes, seed);
      }
      m_network_id.data[0] += 1;
    }

    if (!handleConfig(config)) {
      logger(ERROR, BRIGHT_RED) << "Failed to handle command line";
      return false;
    }

    m_config_folder = config.getConfigFolder();
    m_p2p_state_filename = config.getP2pStateFilename();

    if (!init_config()) {
      logger(ERROR, BRIGHT_RED) << "Failed to init config.";
      return false;
    }

    if (!m_peerlist.init(m_allow_local_ip)) {
      logger(ERROR, BRIGHT_RED) << "Failed to init peerlist.";
      return false;
    }

    for (const auto &p : m_command_line_peers)
    {
      m_peerlist.append_with_peer_white(p);
    }

    //only in case if we really sure that we have external visible ip
    m_have_address = true;
    m_ip_address = 0;
#ifdef ALLOW_DEBUG_COMMANDS
    m_last_stat_request_time = 0;
#endif

    //configure self
    // m_net_server.get_config_object().m_pcommands_handler = this;
    // m_net_server.get_config_object().m_invoke_timeout = cn::P2P_DEFAULT_INVOKE_TIMEOUT;

    //try to bind
    logger(INFO) <<  "Binding on " << m_bind_ip << ":" << m_port;
    m_listeningPort = common::fromString<uint16_t>(m_port);

    m_listener = platform_system::TcpListener(m_dispatcher, platform_system::Ipv4Address(m_bind_ip), static_cast<uint16_t>(m_listeningPort));

    logger(INFO, BRIGHT_GREEN) << "Net service bound on " << m_bind_ip << ":" << m_listeningPort;

    if(m_external_port) {
      logger(INFO) <<  "External port defined as " << m_external_port;
    }

    addPortMapping(logger, m_listeningPort);

    return true;
  }
  //-----------------------------------------------------------------------------------

  cn::CryptoNoteProtocolHandler& NodeServer::get_payload_object()
  {
    return m_payload_handler;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::run() {
    logger(INFO) <<  "Starting node server";

    m_workingContextGroup.spawn(std::bind(&NodeServer::acceptLoop, this));
    m_workingContextGroup.spawn(std::bind(&NodeServer::onIdle, this));
    m_workingContextGroup.spawn(std::bind(&NodeServer::timedSyncLoop, this));
    m_workingContextGroup.spawn(std::bind(&NodeServer::timeoutLoop, this));

    m_stopEvent.wait();

    logger(INFO) <<  "Stopping node server and it's, " << m_connections.size() << " connections...";
    m_workingContextGroup.interrupt();
    m_workingContextGroup.wait();

    logger(INFO) <<  "node server loop stopped successfully";
    return true;
  }

  //-----------------------------------------------------------------------------------

  uint64_t NodeServer::get_connections_count() {
    return m_connections.size();
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::deinit()  {
    return store_config();
  }

  //-----------------------------------------------------------------------------------

  bool NodeServer::store_config()
  {
    try {
      if (!tools::create_directories_if_necessary(m_config_folder)) {
        logger(INFO) <<  "Failed to create data directory: " << m_config_folder;
        return false;
      }

      std::string state_file_path = m_config_folder + "/" + m_p2p_state_filename;
      std::ofstream p2p_data;
      p2p_data.open(state_file_path, std::ios_base::binary | std::ios_base::out | std::ios::trunc);
      if (p2p_data.fail())  {
        logger(INFO) <<  "Failed to save config to file " << state_file_path;
        return false;
      };

      StdOutputStream stream(p2p_data);
      BinaryOutputStreamSerializer a(stream);
      cn::serialize(*this, a);
      return true;
    } catch (const std::exception& e) {
      logger(WARNING) << "store_config failed: " << e.what();
    }

    return false;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::sendStopSignal()  {
    m_stop = true;

    m_dispatcher.remoteSpawn([this] {
      m_stopEvent.set();
      m_payload_handler.stop();
    });

    logger(INFO, BRIGHT_YELLOW) << "Stop signal sent";
    return true;
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::handshake(cn::LevinProtocol& proto, P2pConnectionContext& context, bool just_take_peerlist) {
    COMMAND_HANDSHAKE::request arg;
    COMMAND_HANDSHAKE::response rsp;
    get_local_node_data(arg.node_data);
    m_payload_handler.get_payload_sync_data(arg.payload_data);

    if (!proto.invoke(COMMAND_HANDSHAKE::ID, arg, rsp)) {
      logger(logging::DEBUGGING) << context << "Failed to invoke COMMAND_HANDSHAKE, closing connection.";
      return false;
    }

    context.version = rsp.node_data.version;

    if (rsp.node_data.network_id != m_network_id) {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE Failed, wrong network!  (" << rsp.node_data.network_id << "), closing connection.";
      return false;
    }

    if (rsp.node_data.version < cn::P2P_MINIMUM_VERSION)
    {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE Failed, peer is wrong version! ("
                                 << std::to_string(rsp.node_data.version) << "), closing connection.";
      return false;
    }
    else if ((rsp.node_data.version - cn::P2P_CURRENT_VERSION) >= cn::P2P_UPGRADE_WINDOW)
    {
      logger(logging::WARNING) << context
                               << "COMMAND_HANDSHAKE Warning, your software may be out of date. Please visit: "
                               << "https://github.com/syfernetwork/syfer-core/releases for the latest version.";
    }

    if (!handle_remote_peerlist(rsp.local_peerlist, rsp.node_data.local_time, context)) {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE: failed to handle_remote_peerlist(...), closing connection.";
      return false;
    }

    if (just_take_peerlist) {
      return true;
    }

    if (!m_payload_handler.process_payload_sync_data(rsp.payload_data, context, true)) {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE invoked, but process_payload_sync_data returned false, dropping connection.";
      return false;
    }

    context.peerId = rsp.node_data.peer_id;
    m_peerlist.set_peer_just_seen(rsp.node_data.peer_id, context.m_remote_ip, context.m_remote_port);

    if (rsp.node_data.peer_id == m_config.m_peer_id)  {
      logger(logging::TRACE) << context << "Connection to self detected, dropping connection";
      return false;
    }

    logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE INVOKED OK";
    return true;
  }


  bool NodeServer::timedSync() {
    COMMAND_TIMED_SYNC::request arg = boost::value_initialized<COMMAND_TIMED_SYNC::request>();
    m_payload_handler.get_payload_sync_data(arg.payload_data);
    auto cmdBuf = LevinProtocol::encode<COMMAND_TIMED_SYNC::request>(arg);

    forEachConnection([&cmdBuf](P2pConnectionContext& conn) {
      if (conn.peerId &&
          (conn.m_state == CryptoNoteConnectionContext::state_normal ||
           conn.m_state == CryptoNoteConnectionContext::state_idle)) {
        conn.pushMessage(P2pMessage(P2pMessage::COMMAND, COMMAND_TIMED_SYNC::ID, cmdBuf));
      }
    });

    return true;
  }

  bool NodeServer::handleTimedSyncResponse(const BinaryArray& in, P2pConnectionContext& context) {
    COMMAND_TIMED_SYNC::response rsp;
    if (!LevinProtocol::decode<COMMAND_TIMED_SYNC::response>(in, rsp)) {
      return false;
    }

    if (!handle_remote_peerlist(rsp.local_peerlist, rsp.local_time, context)) {
      logger(logging::DEBUGGING) << context << "COMMAND_TIMED_SYNC: failed to handle_remote_peerlist(...), closing connection.";
      return false;
    }

    if (!context.m_is_income) {
      m_peerlist.set_peer_just_seen(context.peerId, context.m_remote_ip, context.m_remote_port);
    }

    if (!m_payload_handler.process_payload_sync_data(rsp.payload_data, context, false)) {
      return false;
    }

    return true;
  }

  void NodeServer::forEachConnection(const std::function<void(P2pConnectionContext &)> &action)
  {

    // create copy of connection ids because the list can be changed during action
    std::vector<boost::uuids::uuid> connectionIds;
    connectionIds.reserve(m_connections.size());
    for (const auto& c : m_connections) {
      connectionIds.push_back(c.first);
    }

    for (const auto& connId : connectionIds) {
      auto it = m_connections.find(connId);
      if (it != m_connections.end()) {
        action(it->second);
      }
    }
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::is_peer_used(const PeerlistEntry& peer) const {
    if(m_config.m_peer_id == peer.id)
      return true; //dont make connections to ourself

    for (const auto& kv : m_connections) {
      const auto& cntxt = kv.second;
      if(cntxt.peerId == peer.id || (!cntxt.m_is_income && peer.adr.ip == cntxt.m_remote_ip && peer.adr.port == cntxt.m_remote_port)) {
        return true;
      }
    }
    return false;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::is_addr_connected(const NetworkAddress& peer) const {
    for (const auto& conn : m_connections) {
      if (!conn.second.m_is_income && peer.ip == conn.second.m_remote_ip && peer.port == conn.second.m_remote_port) {
        return true;
      }
    }
    return false;
  }

  bool NodeServer::try_to_connect_and_handshake_with_new_peer(const NetworkAddress &na, bool just_take_peerlist, uint64_t last_seen_stamp, PeerType peer_type, uint64_t first_seen_stamp)
  {
    logger(DEBUGGING) << "Connecting to " << na << " (peer_type=" << peer_type << ", last_seen: "
            << (last_seen_stamp ? common::timeIntervalToString(time(nullptr) - last_seen_stamp) : "never") << ")...";

    try {
      platform_system::TcpConnection connection;

      try {
        platform_system::Context<platform_system::TcpConnection> connectionContext(m_dispatcher, [&] {
          platform_system::TcpConnector connector(m_dispatcher);
          return connector.connect(platform_system::Ipv4Address(common::ipAddressToString(na.ip)), static_cast<uint16_t>(na.port));
        });

        platform_system::Context<> timeoutContext(m_dispatcher, [this, &na, &connectionContext] {
          platform_system::Timer(m_dispatcher).sleep(std::chrono::milliseconds(m_config.m_net_config.connection_timeout));
          logger(DEBUGGING) << "Connection to " << na <<" timed out, interrupting it";
          safeInterrupt(connectionContext);
        });

        connection = std::move(connectionContext.get());
      } catch (const platform_system::InterruptedException&) {
        logger(DEBUGGING) << "Connection timed out";
        return false;
      }

      P2pConnectionContext ctx(m_dispatcher, logger.getLogger(), std::move(connection));

      ctx.m_connection_id = boost::uuids::random_generator()();
      ctx.m_remote_ip = na.ip;
      ctx.m_remote_port = na.port;
      ctx.m_is_income = false;
      ctx.m_started = time(nullptr);


      try {
        platform_system::Context<bool> handshakeContext(m_dispatcher, [this, &ctx, &just_take_peerlist] {
          cn::LevinProtocol proto(ctx.connection);
          return handshake(proto, ctx, just_take_peerlist);
        });

        platform_system::Context<> timeoutContext(m_dispatcher, [this, &na, &handshakeContext] {
          // Here we use connection_timeout * 3, one for this handshake, and two for back ping from peer.
          platform_system::Timer(m_dispatcher).sleep(std::chrono::milliseconds(m_config.m_net_config.connection_timeout * 3));
          logger(DEBUGGING) << "Handshake with " << na << " timed out, interrupt it";
          safeInterrupt(handshakeContext);
        });

        if (!handshakeContext.get()) {
          logger(DEBUGGING) << "Failed to HANDSHAKE with peer " << na;
          return false;
        }
      } catch (const platform_system::InterruptedException&) {
        logger(DEBUGGING) << "Handshake timed out";
        return false;
      }

      if (just_take_peerlist) {
        logger(logging::DEBUGGING, logging::BRIGHT_GREEN) << ctx << "CONNECTION HANDSHAKED OK AND CLOSED.";
        return true;
      }

      PeerlistEntry pe_local = boost::value_initialized<PeerlistEntry>();
      pe_local.adr = na;
      pe_local.id = ctx.peerId;
      pe_local.last_seen = time(nullptr);
      m_peerlist.append_with_peer_white(pe_local);

      AnchorPeerlistEntry ape = boost::value_initialized<AnchorPeerlistEntry>();
      ape.adr = na;
      ape.id = ctx.peerId;
      ape.first_seen = first_seen_stamp ? first_seen_stamp : time(nullptr);
      m_peerlist.append_with_peer_anchor(ape);

      if (m_stop) {
        throw platform_system::InterruptedException();
      }

      auto iter = m_connections.emplace(ctx.m_connection_id, std::move(ctx)).first;
      const boost::uuids::uuid& connectionId = iter->first;
      P2pConnectionContext& connectionContext = iter->second;

      m_workingContextGroup.spawn(std::bind(&NodeServer::connectionHandler, this, std::cref(connectionId), std::ref(connectionContext)));

      return true;
    } catch (const platform_system::InterruptedException&) {
      logger(DEBUGGING) << "Connection to new peer interrupted";
      throw;
    } catch (const std::exception& e) {
      logger(DEBUGGING) << "Connection to " << na << " failed: " << e.what();
    }

    return false;
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::make_new_connection_from_peerlist(bool use_white_list)
  {
    size_t local_peers_count = use_white_list ? m_peerlist.get_white_peers_count():m_peerlist.get_gray_peers_count();
    if(!local_peers_count)
      return false;//no peers

    size_t max_random_index = std::min<uint64_t>(local_peers_count -1, 20);

    std::set<size_t> tried_peers;

    size_t try_count = 0;
    size_t rand_count = 0;
    while(rand_count < (max_random_index+1)*3 &&  try_count < 10 && !m_stop) {
      ++rand_count;
      size_t random_index = get_random_index_with_fixed_probability(max_random_index);
      if (!(random_index < local_peers_count)) { logger(ERROR, BRIGHT_RED) << "random_starter_index < peers_local.size() failed!!"; return false; }

      if(tried_peers.count(random_index))
        continue;

      tried_peers.insert(random_index);
      PeerlistEntry pe = boost::value_initialized<PeerlistEntry>();
      bool r = use_white_list ? m_peerlist.get_white_peer_by_index(pe, random_index):m_peerlist.get_gray_peer_by_index(pe, random_index);
      if (!r) { logger(ERROR, BRIGHT_RED) << "Failed to get random peer from peerlist(white:" << use_white_list << ")"; return false; }

      ++try_count;

      if(is_peer_used(pe))
        continue;

      logger(DEBUGGING) << "Selected peer: " << pe.id << " " << pe.adr << " [peer_list=" << (use_white_list ? white : gray)
                        << "] last_seen: " << (pe.last_seen ? common::timeIntervalToString(time(nullptr) - pe.last_seen) : "never");

      if (!try_to_connect_and_handshake_with_new_peer(pe.adr, false, pe.last_seen, use_white_list ? white : gray))
        continue;

      return true;
    }
    return false;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::make_new_connection_from_anchor_peerlist(const std::vector<AnchorPeerlistEntry> &anchor_peerlist)
  {
    for (const auto &pe : anchor_peerlist)
    {
      logger(DEBUGGING) << "Considering connecting (out) to peer: " << pe.id << " " << common::ipAddressToString(pe.adr.ip) << ":" << boost::lexical_cast<std::string>(pe.adr.port);

      if (is_peer_used(pe))
      {
        logger(DEBUGGING) << "Peer is used";
        continue;
      }

      if (is_addr_recently_failed(pe.adr.ip))
      {
        continue;
      }

      logger(DEBUGGING) << "Selected anchor peer: " << pe.id << " " << common::ipAddressToString(pe.adr.ip)
                        << ":" << boost::lexical_cast<std::string>(pe.adr.port)
                        << "[peer_type=" << anchor
                        << "] first_seen: " << common::timeIntervalToString(time(nullptr) - pe.first_seen);

      if (!try_to_connect_and_handshake_with_new_peer(pe.adr, false, 0, anchor, pe.first_seen))
      {
        logger(DEBUGGING) << "Handshake failed";
        continue;
      }

      return true;
    }
    return false;
  }


  bool NodeServer::connections_maker()
  {
    if (!connect_to_peerlist(m_exclusive_peers)) {
      return false;
    }

    if (!m_exclusive_peers.empty()) {
      return true;
    }

    if(!m_peerlist.get_white_peers_count() && m_seed_nodes.size()) {
      size_t try_count = 0;
      size_t current_index = crypto::rand<size_t>() % m_seed_nodes.size();

      while(true) {
        if(try_to_connect_and_handshake_with_new_peer(m_seed_nodes[current_index], true))
          break;

        if(++try_count > m_seed_nodes.size()) {
          logger(ERROR) << "Failed to connect to any of seed peers, continuing without seeds";
          break;
        }
        if(++current_index >= m_seed_nodes.size())
          current_index = 0;
      }
    }

    if (!connect_to_peerlist(m_priority_peers)) return false;

    size_t expected_white_connections = (m_config.m_net_config.connections_count * cn::P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT) / 100;

    size_t conn_count = get_outgoing_connections_count();
    if(conn_count < m_config.m_net_config.connections_count)
    {
      if(conn_count < expected_white_connections)
      {

        if (!make_expected_connections_count(anchor, P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT))
          return false;

        //start from white list
        if (!make_expected_connections_count(white, expected_white_connections))
          return false;
        //and then do grey list
        if (!make_expected_connections_count(gray, m_config.m_net_config.connections_count))
          return false;
      }else
      {
        //start from grey list
        if (!make_expected_connections_count(gray, m_config.m_net_config.connections_count))
          return false;
        //and then do white list
        if (!make_expected_connections_count(white, m_config.m_net_config.connections_count))
          return false;
      }
    }

    return true;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::make_expected_connections_count(PeerType peer_type, size_t expected_connections)
  {
    std::vector<AnchorPeerlistEntry> apl;

    if (peer_type == anchor)
    {
      m_peerlist.get_and_empty_anchor_peerlist(apl);
    }
  
      size_t conn_count = get_outgoing_connections_count();
      //add new connections from white peers
      while (conn_count < expected_connections)
      {
        if (m_stopEvent.get())
          return false;

        if (peer_type == anchor && !make_new_connection_from_anchor_peerlist(apl))
        {
          break;
        }

        if (peer_type == white && !make_new_connection_from_peerlist(true))
        {
          break;
        }

        if (peer_type == gray && !make_new_connection_from_peerlist(false))
        {
          break;
        }
        conn_count = get_outgoing_connections_count();
      }
      return true;
    }

    //-----------------------------------------------------------------------------------
    size_t NodeServer::get_outgoing_connections_count() const
    {
      size_t count = 0;
      for (const auto &cntxt : m_connections)
      {
        if (!cntxt.second.m_is_income)
          ++count;
      }
      return count;
    }

    //-----------------------------------------------------------------------------------
    bool NodeServer::idle_worker()
    {
      try
      {
        m_connections_maker_interval.call(std::bind(&NodeServer::connections_maker, this));
        m_peerlist_store_interval.call(std::bind(&NodeServer::store_config, this));
      } catch (const std::exception& e) {
      logger(DEBUGGING) << "exception in idle_worker: " << e.what();
    }
    return true;
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::fix_time_delta(std::list<PeerlistEntry>& local_peerlist, time_t local_time, int64_t& delta) const
  {
    //fix time delta
    time_t now = 0;
    time(&now);
    delta = now - local_time;

    BOOST_FOREACH(PeerlistEntry& be, local_peerlist)
    {
      if(be.last_seen > uint64_t(local_time))
      {
        logger(DEBUGGING) << "FOUND FUTURE peerlist for entry " << be.adr << " last_seen: " << be.last_seen << ", local_time(on remote node):" << local_time;
        return false;
      }
      be.last_seen += delta;
    }
    return true;
  }

  //-----------------------------------------------------------------------------------

  bool NodeServer::handle_remote_peerlist(const std::list<PeerlistEntry>& peerlist, time_t local_time, const CryptoNoteConnectionContext& context)
  {
    int64_t delta = 0;
    std::list<PeerlistEntry> peerlist_ = peerlist;
    if(!fix_time_delta(peerlist_, local_time, delta))
      return false;
    logger(logging::TRACE) << context << "REMOTE PEERLIST: TIME_DELTA: " << delta << ", remote peerlist size=" << peerlist_.size();
    logger(logging::TRACE) << context << "REMOTE PEERLIST: " <<  print_peerlist_to_string(peerlist_);
    return m_peerlist.merge_peerlist(peerlist_);
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::get_local_node_data(basic_node_data& node_data) const
  {
    node_data.version = cn::P2P_CURRENT_VERSION;
    time_t local_time;
    time(&local_time);
    node_data.local_time = local_time;
    node_data.peer_id = m_config.m_peer_id;
    if(!m_hide_my_port)
      node_data.my_port = m_external_port ? m_external_port : m_listeningPort;
    else
      node_data.my_port = 0;
    node_data.network_id = m_network_id;
    return true;
  }
  //-----------------------------------------------------------------------------------
#ifdef ALLOW_DEBUG_COMMANDS

  bool NodeServer::check_trust(const proof_of_trust &tr) {
    uint64_t local_time = time(NULL);
    uint64_t time_delata = local_time > tr.time ? local_time - tr.time : tr.time - local_time;

    if (time_delata > 24 * 60 * 60) {
      logger(ERROR) << "check_trust failed to check time conditions, local_time=" << local_time << ", proof_time=" << tr.time;
      return false;
    }

    if (m_last_stat_request_time >= tr.time) {
      logger(ERROR) << "check_trust failed to check time conditions, last_stat_request_time=" << m_last_stat_request_time << ", proof_time=" << tr.time;
      return false;
    }

    if (m_config.m_peer_id != tr.peer_id) {
      logger(ERROR) << "check_trust failed: peer_id mismatch (passed " << tr.peer_id << ", expected " << m_config.m_peer_id << ")";
      return false;
    }

    crypto::PublicKey pk;
    common::podFromHex(cn::P2P_STAT_TRUSTED_PUB_KEY, pk);
    crypto::Hash h = get_proof_of_trust_hash(tr);
    if (!crypto::check_signature(h, pk, tr.sign)) {
      logger(ERROR) << "check_trust failed: sign check failed";
      return false;
    }

    //update last request time
    m_last_stat_request_time = tr.time;
    return true;
  }
  //-----------------------------------------------------------------------------------

  int NodeServer::handle_get_stat_info(int command, COMMAND_REQUEST_STAT_INFO::request& arg, COMMAND_REQUEST_STAT_INFO::response& rsp, P2pConnectionContext& context)
  {
    if(!check_trust(arg.tr)) {
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }
    rsp.connections_count = get_connections_count();
    rsp.incoming_connections_count = rsp.connections_count - get_outgoing_connections_count();
    rsp.version = PROJECT_VERSION_LONG;
    rsp.os_version = tools::get_os_version_string();
    m_payload_handler.get_stat_info(rsp.payload_info);
    return 1;
  }
  //-----------------------------------------------------------------------------------

  int NodeServer::handle_get_network_state(int command, COMMAND_REQUEST_NETWORK_STATE::request& arg, COMMAND_REQUEST_NETWORK_STATE::response& rsp, P2pConnectionContext& context)
  {
    if(!check_trust(arg.tr)) {
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }

    for (const auto& cntxt : m_connections) {
      connection_entry ce;
      ce.adr.ip = cntxt.second.m_remote_ip;
      ce.adr.port = cntxt.second.m_remote_port;
      ce.id = cntxt.second.peerId;
      ce.is_income = cntxt.second.m_is_income;
      rsp.connections_list.push_back(ce);
    }

    m_peerlist.get_peerlist_full(rsp.local_peerlist_gray, rsp.local_peerlist_white);
    rsp.my_id = m_config.m_peer_id;
    rsp.local_time = time(NULL);
    return 1;
  }
  //-----------------------------------------------------------------------------------

  int NodeServer::handle_get_peer_id(int command, COMMAND_REQUEST_PEER_ID::request& arg, COMMAND_REQUEST_PEER_ID::response& rsp, P2pConnectionContext& context)
  {
    rsp.my_id = m_config.m_peer_id;
    return 1;
  }
#endif

  //-----------------------------------------------------------------------------------

  void NodeServer::relay_notify_to_all(int command, const BinaryArray& data_buff, const net_connection_id* excludeConnection) {
    net_connection_id excludeId = excludeConnection ? *excludeConnection : boost::value_initialized<net_connection_id>();

    forEachConnection([&excludeId, &command, &data_buff](P2pConnectionContext& conn) {
      if (conn.peerId && conn.m_connection_id != excludeId &&
          (conn.m_state == CryptoNoteConnectionContext::state_normal ||
           conn.m_state == CryptoNoteConnectionContext::state_synchronizing)) {
        conn.pushMessage(P2pMessage(P2pMessage::NOTIFY, command, data_buff));
      }
    });
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::invoke_notify_to_peer(int command, const BinaryArray& buffer, const CryptoNoteConnectionContext& context) {
    auto it = m_connections.find(context.m_connection_id);
    if (it == m_connections.end()) {
      return false;
    }

    it->second.pushMessage(P2pMessage(P2pMessage::NOTIFY, command, buffer));

    return true;
  }

  //-----------------------------------------------------------------------------------
  bool NodeServer::try_ping(const basic_node_data& node_data, const P2pConnectionContext& context) {
    if(!node_data.my_port) {
      return false;
    }

    uint32_t actual_ip =  context.m_remote_ip;
    if(!m_peerlist.is_ip_allowed(actual_ip)) {
      return false;
    }

    auto ip = common::ipAddressToString(actual_ip);
    auto port = node_data.my_port;
    auto peerId = node_data.peer_id;

    try {
      COMMAND_PING::request req;
      COMMAND_PING::response rsp;
      platform_system::Context<> pingContext(m_dispatcher, [this, &ip, &port, &req, &rsp] {
        platform_system::TcpConnector connector(m_dispatcher);
        auto connection = connector.connect(platform_system::Ipv4Address(ip), static_cast<uint16_t>(port));
        LevinProtocol(connection).invoke(COMMAND_PING::ID, req, rsp);
      });

      platform_system::Context<> timeoutContext(m_dispatcher, [this, &context, &ip, &port, &pingContext] {
        platform_system::Timer(m_dispatcher).sleep(std::chrono::milliseconds(m_config.m_net_config.connection_timeout * 2));
        logger(DEBUGGING) << context << "Back ping timed out" << ip << ":" << port;
        safeInterrupt(pingContext);
      });

      pingContext.get();

      if (rsp.status != PING_OK_RESPONSE_STATUS_TEXT || peerId != rsp.peer_id) {
        logger(DEBUGGING) << context << "Back ping invoke wrong response \"" << rsp.status << "\" from" << ip
                                   << ":" << port << ", hsh_peer_id=" << peerId << ", rsp.peer_id=" << rsp.peer_id;
        return false;
      }
    } catch (const std::exception& e) {
      logger(DEBUGGING) << context << "Back ping connection to " << ip << ":" << port << " failed: " << e.what();
      return false;
    }

    return true;
  }

  //-----------------------------------------------------------------------------------
  int NodeServer::handle_timed_sync(int command, const COMMAND_TIMED_SYNC::request& arg, COMMAND_TIMED_SYNC::response& rsp, P2pConnectionContext& context)
  {
    if(!m_payload_handler.process_payload_sync_data(arg.payload_data, context, false)) {
      logger(logging::DEBUGGING) << context << "Failed to process_payload_sync_data(), dropping connection";
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }

    //fill response
    rsp.local_time = time(nullptr);
    m_peerlist.get_peerlist_head(rsp.local_peerlist);
    m_payload_handler.get_payload_sync_data(rsp.payload_data);
    logger(logging::TRACE) << context << "COMMAND_TIMED_SYNC";
    return 1;
  }
  //-----------------------------------------------------------------------------------

  int NodeServer::handle_handshake(int command, const COMMAND_HANDSHAKE::request& arg, COMMAND_HANDSHAKE::response& rsp, P2pConnectionContext& context)
  {
    context.version = arg.node_data.version;

    if (arg.node_data.network_id != m_network_id) {
      logger(logging::INFO) << context << "WRONG NETWORK AGENT CONNECTED! id=" << arg.node_data.network_id;
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }

    if (arg.node_data.version < cn::P2P_MINIMUM_VERSION) {
      logger(logging::DEBUGGING) << context << "UNSUPPORTED NETWORK AGENT VERSION CONNECTED! version=" << std::to_string(arg.node_data.version);
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    } else if (arg.node_data.version > cn::P2P_CURRENT_VERSION) {
      logger(logging::WARNING) << context << "Warning, your software may be out of date. Please upgrare to the latest version.";
    }

    if(!context.m_is_income) {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE came not from incoming connection";
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }

    if(context.peerId) {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE came, but seems that connection already have associated peer_id (double COMMAND_HANDSHAKE?)";
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }

    if(!m_payload_handler.process_payload_sync_data(arg.payload_data, context, true))  {
      logger(logging::DEBUGGING) << context << "COMMAND_HANDSHAKE came, but process_payload_sync_data returned false, dropping connection.";
      context.m_state = CryptoNoteConnectionContext::state_shutdown;
      return 1;
    }
    //associate peer_id with this connection
    context.peerId = arg.node_data.peer_id;

    if(arg.node_data.peer_id != m_config.m_peer_id && arg.node_data.my_port) {
      PeerIdType peer_id_l = arg.node_data.peer_id;
      uint32_t port_l = arg.node_data.my_port;

      if (try_ping(arg.node_data, context)) {
          //called only(!) if success pinged, update local peerlist
          PeerlistEntry pe;
          pe.adr.ip = context.m_remote_ip;
          pe.adr.port = port_l;
          pe.last_seen = time(nullptr);
          pe.id = peer_id_l;
          m_peerlist.append_with_peer_white(pe);

          logger(logging::TRACE) << context << "BACK PING SUCCESS, " << common::ipAddressToString(context.m_remote_ip) << ":" << port_l << " added to whitelist";
      }
    }

    //fill response
    m_peerlist.get_peerlist_head(rsp.local_peerlist);
    get_local_node_data(rsp.node_data);
    m_payload_handler.get_payload_sync_data(rsp.payload_data);

    logger(logging::DEBUGGING, logging::BRIGHT_GREEN) << "COMMAND_HANDSHAKE";
    return 1;
  }
  //-----------------------------------------------------------------------------------

  int NodeServer::handle_ping(int command, const COMMAND_PING::request& arg, COMMAND_PING::response& rsp, const P2pConnectionContext& context) const
  {
    logger(logging::TRACE) << context << "COMMAND_PING";
    rsp.status = PING_OK_RESPONSE_STATUS_TEXT;
    rsp.peer_id = m_config.m_peer_id;
    return 1;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::log_peerlist() const
  {
    std::list<PeerlistEntry> pl_wite;
    std::list<PeerlistEntry> pl_gray;
    m_peerlist.get_peerlist_full(pl_gray, pl_wite);
    logger(INFO) << std::endl
                 << "Peerlist white:\n"
                 << print_peerlist_to_string(pl_wite) << std::endl
                 << "Peerlist gray:\n"
                 << print_peerlist_to_string(pl_gray);
    return true;
  }
  //-----------------------------------------------------------------------------------

  bool NodeServer::log_connections() const {
    logger(INFO) <<  "Connections: \r\n" << print_connections_container() ;
    return true;
  }
  //-----------------------------------------------------------------------------------

  std::string NodeServer::print_connections_container() const {

    std::stringstream ss;

    for (const auto& cntxt : m_connections) {
      ss << common::ipAddressToString(cntxt.second.m_remote_ip) << ":" << cntxt.second.m_remote_port
        << " \t\tpeer_id " << cntxt.second.peerId
        << " \t\tconn_id " << cntxt.second.m_connection_id << (cntxt.second.m_is_income ? " INC" : " OUT")
        << std::endl;
    }

    return ss.str();
  }
  //-----------------------------------------------------------------------------------

  void NodeServer::on_connection_new(P2pConnectionContext& context)
  {
    logger(TRACE) << context << "NEW CONNECTION";
    m_payload_handler.onConnectionOpened(context);
  }
  //-----------------------------------------------------------------------------------

  void NodeServer::on_connection_close(P2pConnectionContext& context)
  {

    if (!m_stopEvent.get() && !context.m_is_income)
    {
      NetworkAddress na;
      na.ip = context.m_remote_ip;
      na.port = context.m_remote_port;

      m_peerlist.remove_from_peer_anchor(na);
    }

    logger(TRACE) << context << "CLOSE CONNECTION";
    m_payload_handler.onConnectionClosed(context);
  }

  bool NodeServer::is_priority_node(const NetworkAddress& na)
  {
    return
      (std::find(m_priority_peers.begin(), m_priority_peers.end(), na) != m_priority_peers.end()) ||
      (std::find(m_exclusive_peers.begin(), m_exclusive_peers.end(), na) != m_exclusive_peers.end());
  }

  bool NodeServer::connect_to_peerlist(const std::vector<NetworkAddress>& peers)
  {
    for(const auto& na: peers) {
      if (!is_addr_connected(na)) {
        try_to_connect_and_handshake_with_new_peer(na);
      }
    }

    return true;
  }

  bool NodeServer::parse_peers_and_add_to_container(const boost::program_options::variables_map& vm,
    const command_line::arg_descriptor<std::vector<std::string> > & arg, std::vector<NetworkAddress>& container) const
  {
    std::vector<std::string> perrs = command_line::get_arg(vm, arg);

    for(const std::string& pr_str: perrs) {
      NetworkAddress na;
      if (!parse_peer_from_string(na, pr_str)) {
        logger(ERROR, BRIGHT_RED) << "Failed to parse address from string: " << pr_str;
        return false;
      }
      container.push_back(na);
    }

    return true;
  }

  void NodeServer::acceptLoop() {
    for (;;) {
      try {
        P2pConnectionContext ctx(m_dispatcher, logger.getLogger(), m_listener.accept());
        ctx.m_connection_id = boost::uuids::random_generator()();
        ctx.m_is_income = true;
        ctx.m_started = time(nullptr);

        auto addressAndPort = ctx.connection.getPeerAddressAndPort();
        ctx.m_remote_ip = hostToNetwork(addressAndPort.first.getValue());
        ctx.m_remote_port = addressAndPort.second;

        auto iter = m_connections.emplace(ctx.m_connection_id, std::move(ctx)).first;
        const boost::uuids::uuid& connectionId = iter->first;
        P2pConnectionContext& connection = iter->second;

        m_workingContextGroup.spawn(std::bind(&NodeServer::connectionHandler, this, std::cref(connectionId), std::ref(connection)));
      } catch (const platform_system::InterruptedException&) {
        logger(DEBUGGING) << "NodeServer::acceptLoop() is interrupted";
        break;
      } catch (const std::exception& e) {
        logger(DEBUGGING) << "Exception in NodeServer::acceptLoop(): " << e.what();
      }
    }

    logger(DEBUGGING) << "NodeServer::acceptLoop() finished";
  }

  void NodeServer::onIdle() {
    logger(DEBUGGING) << "NodeServer::onIdle() started";

    try {
      while (!m_stop) {
        idle_worker();
        m_payload_handler.on_idle();
        m_idleTimer.sleep(std::chrono::seconds(1));
      }
    } catch (const platform_system::InterruptedException&) {
      logger(DEBUGGING) << "NodeServer::onIdle() is interrupted";
    } catch (const std::exception& e) {
      logger(DEBUGGING) << "Exception in NodeServer::onIdle(): " << e.what();
    }

    logger(DEBUGGING) << "NodeServer::onIdle() finished";
  }

  void NodeServer::timeoutLoop() {
    try {
      while (!m_stop) {
        m_timeoutTimer.sleep(std::chrono::seconds(10));
        auto now = P2pConnectionContext::Clock::now();

        for (auto& kv : m_connections) {
          auto& ctx = kv.second;
          if (ctx.writeDuration(now) > P2P_DEFAULT_INVOKE_TIMEOUT) {
            logger(DEBUGGING) << ctx << "write operation timed out, stopping connection";
            safeInterrupt(ctx);
          }
        }
      }
    } catch (const platform_system::InterruptedException&) {
      logger(DEBUGGING) << "NodeServer::timeoutLoop() is interrupted";
    } catch (const std::exception& e) {
      logger(DEBUGGING) << "Exception in NodeServer::timeoutLoop(): " << e.what();
    }
  }

  void NodeServer::timedSyncLoop() {
    try {
      for (;;) {
        m_timedSyncTimer.sleep(std::chrono::seconds(P2P_DEFAULT_HANDSHAKE_INTERVAL));
        timedSync();
      }
    } catch (const platform_system::InterruptedException&) {
      logger(DEBUGGING) << "NodeServer::timedSyncLoop() is interrupted";
    } catch (const std::exception& e) {
      logger(DEBUGGING) << "Exception in NodeServer::timedSyncLoop(): " << e.what();
    }

    logger(DEBUGGING) << "NodeServer::timedSyncLoop() finished";
  }

  void NodeServer::connectionHandler(const boost::uuids::uuid& connectionId, P2pConnectionContext& ctx) {
    // This inner context is necessary in order to stop connection handler at any moment
    platform_system::Context<> context(m_dispatcher, [this, &connectionId, &ctx] {
      platform_system::Context<> writeContext(m_dispatcher, std::bind(&NodeServer::writeHandler, this, std::ref(ctx)));

      try {
        on_connection_new(ctx);

        LevinProtocol proto(ctx.connection);
        LevinProtocol::Command cmd;

        for (;;) {
          if (ctx.m_state == CryptoNoteConnectionContext::state_sync_required) {
            ctx.m_state = CryptoNoteConnectionContext::state_synchronizing;
            m_payload_handler.start_sync(ctx);
          } else if (ctx.m_state == CryptoNoteConnectionContext::state_pool_sync_required) {
            ctx.m_state = CryptoNoteConnectionContext::state_normal;
            m_payload_handler.requestMissingPoolTransactions(ctx);
          }

          if (!proto.readCommand(cmd)) {
            break;
          }

          BinaryArray response;
          bool handled = false;
          auto retcode = handleCommand(cmd, response, ctx, handled);

          // send response
          if (cmd.needReply()) {
            if (!handled) {
              retcode = static_cast<int32_t>(LevinError::ERROR_CONNECTION_HANDLER_NOT_DEFINED);
              response.clear();
            }

            ctx.pushMessage(P2pMessage(P2pMessage::REPLY, cmd.command, response, retcode));
          }

          if (ctx.m_state == CryptoNoteConnectionContext::state_shutdown) {
            break;
          }
        }
      } catch (const platform_system::InterruptedException&) {
        logger(DEBUGGING) << ctx << "NodeServer::connectionHandler() inner context is interrupted";
      } catch (const std::exception& e) {
        logger(DEBUGGING) << ctx << "Exception in NodeServer::connectionHandler(): " << e.what();
      }

      safeInterrupt(ctx);
      safeInterrupt(writeContext);
      writeContext.wait();

      on_connection_close(ctx);
      m_connections.erase(connectionId);
    });

    ctx.context = &context;

    try {
      context.get();
    } catch (const platform_system::InterruptedException&) {
      logger(DEBUGGING) << "NodeServer::connectionHandler() is interrupted";
    }
  }

  void NodeServer::writeHandler(P2pConnectionContext& ctx) const {
    logger(DEBUGGING) << ctx << "NodeServer::writeHandler() started";

    try {
      LevinProtocol proto(ctx.connection);

      for (;;) {
        auto msgs = ctx.popBuffer();
        if (msgs.empty()) {
          break;
        }

        for (const auto& msg : msgs) {
          logger(DEBUGGING) << ctx << "msg " << msg.type << ':' << msg.command;
          switch (msg.type) {
          case P2pMessage::COMMAND:
            proto.sendMessage(msg.command, msg.buffer, true);
            break;
          case P2pMessage::NOTIFY:
            proto.sendMessage(msg.command, msg.buffer, false);
            break;
          case P2pMessage::REPLY:
            proto.sendReply(msg.command, msg.buffer, msg.returnCode);
            break;
          default:
            assert(false);
          }
        }
      }
    } catch (const platform_system::InterruptedException&) {
      // connection stopped
      logger(DEBUGGING) << ctx << "NodeServer::writeHandler() is interrupted";
    } catch (const std::exception& e) {
      logger(DEBUGGING) << ctx << "NodeServer::writeHandler() error during write: " << e.what();
      safeInterrupt(ctx); // stop connection on write error
    }

    logger(DEBUGGING) << ctx << "NodeServer::writeHandler() finished";
  }

  template<typename T>
  void NodeServer::safeInterrupt(T& obj) const {
    try {
      obj.interrupt();
    } catch (const std::exception& e) {
      logger(DEBUGGING) << "NodeServer::safeInterrupt() throws exception: " << e.what();
    } catch (...) {
      logger(DEBUGGING) << "NodeServer::safeInterrupt() throws unknown exception";
    }
  }
  }
