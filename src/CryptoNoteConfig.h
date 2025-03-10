// Copyright (c) 2011-2017 The Cryptonote Developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <initializer_list>

namespace cn
{
	namespace parameters
	{

		const uint64_t CRYPTONOTE_MAX_BLOCK_NUMBER = 500000000;
		const size_t CRYPTONOTE_MAX_BLOCK_BLOB_SIZE = 500000000;
		const size_t CRYPTONOTE_MAX_TX_SIZE = 1000000000;
		const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX = 0x3119; /* Sf address prefix */
		const size_t CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW = 10;			 /* 20 minutes */
		const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT = 60 * 60 * 2; /* two hours */
		const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1 = 360;		 /* changed for LWMA3 */
		const uint64_t CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10;		 /* 20 minutes */

		const size_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW = 30;
		const size_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1 = 11; /* changed for LWMA3 */

		const uint64_t MONEY_SUPPLY = UINT64_C(9999000000000000); /* max supply: 250M (Consensus II) */

		const uint32_t ZAWY_DIFFICULTY_BLOCK_INDEX = 0;
		const size_t ZAWY_DIFFICULTY_FIX = 1;
		const uint8_t ZAWY_DIFFICULTY_BLOCK_VERSION = 0;

		const size_t CRYPTONOTE_REWARD_BLOCKS_WINDOW = 100;
		const size_t CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE = 100000; /* size of block in bytes, after which reward is calculated using block size */
		const size_t CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE = 600;
		const size_t CRYPTONOTE_DISPLAY_DECIMAL_POINT = 6;

		const uint64_t POINT = UINT64_C(1000);
		const uint64_t COIN = UINT64_C(1000000);			  /* smallest atomic unit */
		const uint64_t MINIMUM_FEE = UINT64_C(10);			  /* 0.000010 SYFR */
		const uint64_t MINIMUM_FEE_V1 = UINT64_C(100);		  /* 0.000100 SYFR */
		const uint64_t MINIMUM_FEE_V2 = UINT64_C(1000);		  /* 0.001000 SYFR */
		const uint64_t MINIMUM_FEE_BANKING = UINT64_C(1000);  /* 0.001000 SYFR */
		const uint64_t DEFAULT_DUST_THRESHOLD = UINT64_C(10); /* 0.000010 SYFR */

		const uint64_t DIFFICULTY_TARGET = 120;												 /* two minutes */
		const uint64_t EXPECTED_NUMBER_OF_BLOCKS_PER_DAY = 24 * 60 * 60 / DIFFICULTY_TARGET; /* 720 blocks */
		const size_t DIFFICULTY_WINDOW = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;
		const size_t DIFFICULTY_WINDOW_V1 = DIFFICULTY_WINDOW;
		const size_t DIFFICULTY_WINDOW_V2 = DIFFICULTY_WINDOW;
		const size_t DIFFICULTY_WINDOW_V3 = 60; /* changed for LWMA3 */
		const size_t DIFFICULTY_WINDOW_V4 = 60;
		const size_t DIFFICULTY_BLOCKS_COUNT = DIFFICULTY_WINDOW_V3 + 1;	/* added for LWMA3 */
		const size_t DIFFICULTY_BLOCKS_COUNT_V1 = DIFFICULTY_WINDOW_V4 + 1; /* added for LWMA1 */
		const size_t DIFFICULTY_CUT = 60;									/* timestamps to cut after sorting */
		const size_t DIFFICULTY_CUT_V1 = DIFFICULTY_CUT;
		const size_t DIFFICULTY_CUT_V2 = DIFFICULTY_CUT;
		const size_t DIFFICULTY_LAG = 15;
		const size_t DIFFICULTY_LAG_V1 = DIFFICULTY_LAG;
		const size_t DIFFICULTY_LAG_V2 = DIFFICULTY_LAG;
		const size_t MINIMUM_MIXIN = 0;

		static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Bad DIFFICULTY_WINDOW or DIFFICULTY_CUT");

		const uint64_t DEPOSIT_MIN_AMOUNT = 1 * COIN;
		const uint32_t DEPOSIT_MIN_TERM = 5040;				 /* one week */
		const uint32_t DEPOSIT_MAX_TERM = 1 * 12 * 21900;	 /* legacy deposts - one year */
		const uint32_t DEPOSIT_MAX_TERM_V1 = 64800 * 20;	 /* five years */
		const uint32_t DEPOSIT_MIN_TERM_V3 = 21900;			 /* consensus 2019 - one month */
		const uint32_t DEPOSIT_MAX_TERM_V3 = 1 * 12 * 21900; /* consensus 2019 - one year */
		const uint32_t DEPOSIT_HEIGHT_V3 = 580;			 /* consensus 2019 - deposts v3.0 */
		const uint64_t DEPOSIT_MIN_TOTAL_RATE_FACTOR = 0;	 /* constant rate */
		const uint64_t DEPOSIT_MAX_TOTAL_RATE = 4;			 /* legacy deposits */
		const uint32_t DEPOSIT_HEIGHT_V4 = 610;			 /* enforce deposit terms */
		const uint32_t BLOCK_WITH_MISSING_INTEREST = 0; /*  */

		static_assert(DEPOSIT_MIN_TERM > 0, "Bad DEPOSIT_MIN_TERM");
		static_assert(DEPOSIT_MIN_TERM <= DEPOSIT_MAX_TERM, "Bad DEPOSIT_MAX_TERM");
		static_assert(DEPOSIT_MIN_TERM * DEPOSIT_MAX_TOTAL_RATE > DEPOSIT_MIN_TOTAL_RATE_FACTOR, "Bad DEPOSIT_MIN_TOTAL_RATE_FACTOR or DEPOSIT_MAX_TOTAL_RATE");

		const uint64_t MULTIPLIER_FACTOR = 100;		 /* legacy deposits */
		const uint32_t END_MULTIPLIER_BLOCK = 101; /* legacy deposits */

		const size_t MAX_BLOCK_SIZE_INITIAL = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 10;
		const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR = 100 * 1024;
		const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR = 365 * 24 * 60 * 60 / DIFFICULTY_TARGET;

		const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS = 1;
		const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS = DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;

		const size_t CRYPTONOTE_MAX_TX_SIZE_LIMIT = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE; /* maximum transaction size */
		const size_t CRYPTONOTE_OPTIMIZE_SIZE = 100;																					/* proportional to CRYPTONOTE_MAX_TX_SIZE_LIMIT */

		const uint64_t CRYPTONOTE_MEMPOOL_TX_LIVETIME = (60 * 60 * 12);					/* 1 hour in seconds */
		const uint64_t CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME = (60 * 60 * 12);	/* 24 hours in seconds */
		const uint64_t CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL = 7; /* CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL * CRYPTONOTE_MEMPOOL_TX_LIVETIME  = time to forget tx */

		const size_t FUSION_TX_MAX_SIZE = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 30 / 100;
		const size_t FUSION_TX_MIN_INPUT_COUNT = 12;
		const size_t FUSION_TX_MIN_IN_OUT_COUNT_RATIO = 4;

		const uint64_t UPGRADE_HEIGHT = 1;
		const uint64_t UPGRADE_HEIGHT_V2 = 1;
        const uint64_t UPGRADE_HEIGHT_V3 = 101;	  /* Cryptonight-Fast */
		const uint64_t UPGRADE_HEIGHT_V4 = 201;	  /* MixIn 2 */
		const uint64_t UPGRADE_HEIGHT_V5 = 301;	  /* Deposits 2.0, Investments 1.0 */
		const uint64_t UPGRADE_HEIGHT_V6 = 401;	  /* LWMA3 */
		const uint64_t UPGRADE_HEIGHT_V7 = 501;	  /* Cryptoight Syfer */
		const uint64_t UPGRADE_HEIGHT_V8 = 601;	  /* CN-GPU, Halving */
		const uint64_t UPGRADE_HEIGHT_V9 = 6000;	  /* SWAP */
		const unsigned UPGRADE_VOTING_THRESHOLD = 90; // percent
		const size_t UPGRADE_VOTING_WINDOW = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;
		const size_t UPGRADE_WINDOW = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY;


		const uint64_t TESTNET_UPGRADE_HEIGHT = 1;
		const uint64_t TESTNET_UPGRADE_HEIGHT_V2 = 1;
		const uint64_t TESTNET_UPGRADE_HEIGHT_V3 = 12;	  /* Cryptonight-Fast */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V4 = 24;	  /* MixIn 2 */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V5 = 36;	  /* Deposits 2.0, Investments 1.0 */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V6 = 48;	  /* LWMA3 */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V7 = 60;	  /* Cryptoight Syfer */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V8 = 72;	  /* LWMA1, CN-GPU, Halving */
		const uint64_t TESTNET_UPGRADE_HEIGHT_V9 = 100;	  /* SWAP */

		const uint32_t TESTNET_DEPOSIT_MIN_TERM_V3 = 30;		/* testnet deposits 1 month -> 1 hour */
		const uint32_t TESTNET_DEPOSIT_MAX_TERM_V3 = 12 * 30;	/* testnet deposits 1 year -> 12 hour */
		const uint32_t TESTNET_DEPOSIT_HEIGHT_V3 = 60;		
		const uint32_t TESTNET_DEPOSIT_HEIGHT_V4 = 300000;
		const uint32_t TESTNET_BLOCK_WITH_MISSING_INTEREST = 0; /* testnet is not impacted */

		static_assert(0 < UPGRADE_VOTING_THRESHOLD && UPGRADE_VOTING_THRESHOLD <= 100, "Bad UPGRADE_VOTING_THRESHOLD");
		static_assert(UPGRADE_VOTING_WINDOW > 1, "Bad UPGRADE_VOTING_WINDOW");

		const char CRYPTONOTE_BLOCKS_FILENAME[] = "blocks.dat";
		const char CRYPTONOTE_BLOCKINDEXES_FILENAME[] = "blockindexes.dat";
		const char CRYPTONOTE_BLOCKSCACHE_FILENAME[] = "blockscache.dat";
		const char CRYPTONOTE_POOLDATA_FILENAME[] = "poolstate.bin";
		const char P2P_NET_DATA_FILENAME[] = "p2pstate.bin";
		const char CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME[] = "blockchainindices.dat";
		const char MINER_CONFIG_FILE_NAME[] = "miner_conf.json";

	} // namespace parameters

	const uint64_t START_BLOCK_REWARD = (UINT64_C(5000) * parameters::POINT);  // start reward (Consensus I)
	const uint64_t FOUNDATION_TRUST = (UINT64_C(1000000) * parameters::COIN); // locked funds to secure network  (Consensus II)
	const uint64_t MAX_BLOCK_REWARD = (UINT64_C(15) * parameters::COIN);	   // max reward (Consensus I)
	const uint64_t MAX_BLOCK_REWARD_V1 = (UINT64_C(6) * parameters::COIN);
	const uint64_t MAX_BLOCK_REWARD_V2 = (UINT64_C(12) * parameters::COIN);
	const uint64_t REWARD_INCREASE_INTERVAL = (UINT64_C(21900));			   // aprox. 1 month (+ 0.25 SYFR increment per month)

	const char BLOCKCHAIN_DIR[] = "syfer";
	const char GENESIS_COINBASE_TX_HEX[] = "010a01ff0001c096b102029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807121017d6775185749e95ac2d70cae3f29e0e46f430ab648abbe9fdc61d8e7437c60f8";
	const uint32_t GENESIS_NONCE = 7000;
	const uint64_t GENESIS_TIMESTAMP = 1673183142;

	const uint64_t TESTNET_GENESIS_TIMESTAMP = 1632048808;

	const uint8_t TRANSACTION_VERSION_1 = 1;
	const uint8_t TRANSACTION_VERSION_2 = 2;
	const uint8_t BLOCK_MAJOR_VERSION_1 = 1; // (Consensus I)
	const uint8_t BLOCK_MAJOR_VERSION_2 = 2; // (Consensus II)
	const uint8_t BLOCK_MAJOR_VERSION_3 = 3; // (Consensus III)
	const uint8_t BLOCK_MAJOR_VERSION_4 = 4; // LWMA3
	const uint8_t BLOCK_MAJOR_VERSION_7 = 7; /* Cryptonight Syfer */
	const uint8_t BLOCK_MAJOR_VERSION_8 = 8; /* LWMA1, CN-GPU, Halving */
	const uint8_t BLOCK_MAJOR_VERSION_9 = 9; /* SWAP */
	const uint8_t BLOCK_MINOR_VERSION_0 = 0;
	const uint8_t BLOCK_MINOR_VERSION_1 = 1;

	const size_t BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT = 10000; // by default, blocks ids count in synchronizing
	const size_t BLOCKS_SYNCHRONIZING_DEFAULT_COUNT = 128;		 // by default, blocks count in blocks downloading
	const size_t COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT = 1000;
    const size_t COMMAND_RPC_GET_OBJECTS_MAX_COUNT = 1000;

	const int P2P_DEFAULT_PORT = 16000;
	const int RPC_DEFAULT_PORT = 17000;
    const int PAYMENT_GATE_DEFAULT_PORT = 8075;

	const int TESTNET_P2P_DEFAULT_PORT = 15500;
	const int TESTNET_RPC_DEFAULT_PORT = 16600;
    const int TESTNET_PAYMENT_GATE_DEFAULT_PORT = 8770;

	/* P2P Network Configuration Section - This defines our current P2P network version
	and the minimum version for communication between nodes */
	const uint8_t P2P_VERSION_1 = 1;
	const uint8_t P2P_VERSION_2 = 2;
	const uint8_t P2P_CURRENT_VERSION = 1;
	const uint8_t P2P_MINIMUM_VERSION = 1;
	const uint8_t P2P_UPGRADE_WINDOW = 2;

	// This defines the minimum P2P version required for lite blocks propogation
	const uint8_t P2P_LITE_BLOCKS_PROPOGATION_VERSION = 3;

	const size_t P2P_LOCAL_WHITE_PEERLIST_LIMIT = 1000;
	const size_t P2P_LOCAL_GRAY_PEERLIST_LIMIT = 5000;

	const size_t P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE = 64 * 1024 * 1024; // 64MB
	const uint32_t P2P_DEFAULT_CONNECTIONS_COUNT = 8;
	const size_t P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT = 2;
	const size_t P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT = 70; // percent
	const uint32_t P2P_DEFAULT_HANDSHAKE_INTERVAL = 60;			 // seconds
	const uint32_t P2P_DEFAULT_PACKET_MAX_SIZE = 50000000;		 // 50000000 bytes maximum packet size
	const uint32_t P2P_DEFAULT_PEERS_IN_HANDSHAKE = 250;
	const uint32_t P2P_DEFAULT_CONNECTION_TIMEOUT = 5000;	   // 5 seconds
	const uint32_t P2P_DEFAULT_PING_CONNECTION_TIMEOUT = 2000; // 2 seconds
	const uint64_t P2P_DEFAULT_INVOKE_TIMEOUT = 60 * 2 * 1000; // 2 minutes
	const size_t P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT = 5000;  // 5 seconds
	const char P2P_STAT_TRUSTED_PUB_KEY[] = "f7061e9a5f0d305492fde49c9bfbaa52ac60afdc46304642b460a9ea34bf7a4e";

	// Seed Nodes
	const std::initializer_list<const char *> SEED_NODES = {
		"23.92.65.226:16000", // 
		"107.155.108.173:16000",	 //  
		"104.251.217.80:16000"	 // 
	};

	const std::initializer_list<const char *> TESTNET_SEED_NODES = {
		"23.92.65.226:15500",
		"23.92.65.226:15501"
	};

	struct CheckpointData
	{
		uint32_t height;
		const char *blockId;
	};

#ifdef __GNUC__
	__attribute__((unused))
#endif

	// Blockchain Checkpoints:
	// {<block height>, "<block hash>"},
	const std::initializer_list<CheckpointData>
		CHECKPOINTS = {
		{0, "6b15db6b4d419de4d9df06f9e14e0a8548cf058dc8594794ac1141cc615f3bd1"},
			{10000, "7a210e2af485b2865706087d2a6750b46116bffacb94d99c8948f6762fe85267"},
			{50000, "152a8fe265fc0e769aad8e3d5d7a09a8ec0c5e4061eea01372addada979991c8"}
	};

    const std::initializer_list<CheckpointData> TESTNET_CHECKPOINTS = {
        //{0, "850ac16022f4dddab624fad3f9049dba80592c8ea51a5dff19fefeb386e536b1"}
        };

} // namespace cn

