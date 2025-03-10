// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation 
// Copyright (c) 2018-2022 Conceal Network Copyright (c) 2023 Syfer Network 
//
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Currency.h"
#include <cctype>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include "../Common/Base58.h"
#include "../Common/int-util.h"
#include "../Common/StringTools.h"

#include "CryptoNoteConfig.h"
#include "Account.h"
#include "CryptoNoteBasicImpl.h"
#include "CryptoNoteFormatUtils.h"
#include "CryptoNoteTools.h"
#include "TransactionExtra.h"
#include "UpgradeDetector.h"

#undef ERROR

using namespace logging;
using namespace common;

namespace cn
{

  const std::vector<uint64_t> Currency::PRETTY_AMOUNTS = {
      1, 2, 3, 4, 5, 6, 7, 8, 9,
      10, 20, 30, 40, 50, 60, 70, 80, 90,
      100, 200, 300, 400, 500, 600, 700, 800, 900,
      1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
      10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
      100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
      1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
      10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000,
      100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000,
      1000000000, 2000000000, 3000000000, 4000000000, 5000000000, 6000000000, 7000000000, 8000000000, 9000000000,
      10000000000, 20000000000, 30000000000, 40000000000, 50000000000, 60000000000, 70000000000, 80000000000, 90000000000,
      100000000000, 200000000000, 300000000000, 400000000000, 500000000000, 600000000000, 700000000000, 800000000000, 900000000000,
      1000000000000, 2000000000000, 3000000000000, 4000000000000, 5000000000000, 6000000000000, 7000000000000, 8000000000000, 9000000000000,
      10000000000000, 20000000000000, 30000000000000, 40000000000000, 50000000000000, 60000000000000, 70000000000000, 80000000000000, 90000000000000,
      100000000000000, 200000000000000, 300000000000000, 400000000000000, 500000000000000, 600000000000000, 700000000000000, 800000000000000, 900000000000000,
      1000000000000000, 2000000000000000, 3000000000000000, 4000000000000000, 5000000000000000, 6000000000000000, 7000000000000000, 8000000000000000, 9000000000000000,
      10000000000000000, 20000000000000000, 30000000000000000, 40000000000000000, 50000000000000000, 60000000000000000, 70000000000000000, 80000000000000000, 90000000000000000,
      100000000000000000, 200000000000000000, 300000000000000000, 400000000000000000, 500000000000000000, 600000000000000000, 700000000000000000, 800000000000000000, 900000000000000000,
      1000000000000000000, 2000000000000000000, 3000000000000000000, 4000000000000000000, 5000000000000000000, 6000000000000000000, 7000000000000000000, 8000000000000000000, 9000000000000000000,
      10000000000000000000ULL};

  const std::vector<uint64_t> Currency::REWARD_INCREASING_FACTOR = {
      0, 250000, 500000, 750000,
      1000000, 1250000, 1500000, 1750000,
      2000000, 2250000, 2500000, 2750000,
      3000000, 3250000, 3500000, 3750000,
      4000000, 4250000, 4500000, 4750000,
      5000000, 5250000, 5500000, 5750000,
      6000000, 6250000, 6500000, 6750000,
      7000000, 7250000, 7500000, 7750000,
      8000000, 8250000, 8500000, 8750000,
      9000000, 9250000, 9500000, 9750000,
      10000000, 10250000, 10500000, 10750000,
      11000000, 11250000, 11500000, 11750000,
      12000000};

  bool Currency::init()
  {
    if (!generateGenesisBlock())
    {
      logger(ERROR, BRIGHT_RED) << "Failed to generate genesis block";
      return false;
    }

    if (!get_block_hash(m_genesisBlock, m_genesisBlockHash))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to get genesis block hash";
      return false;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::generateGenesisBlock()
  {
    m_genesisBlock = boost::value_initialized<Block>();

    // Hard code coinbase tx in genesis block, because "tru" generating tx use random, but genesis should be always the same
    std::string genesisCoinbaseTxHex = GENESIS_COINBASE_TX_HEX;
    BinaryArray minerTxBlob;

    bool r =
        fromHex(genesisCoinbaseTxHex, minerTxBlob) &&
        fromBinaryArray(m_genesisBlock.baseTransaction, minerTxBlob);

    if (!r)
    {
      logger(ERROR, BRIGHT_RED) << "failed to parse coinbase tx from hard coded blob";
      return false;
    }

    m_genesisBlock.majorVersion = BLOCK_MAJOR_VERSION_1;
    m_genesisBlock.minorVersion = BLOCK_MINOR_VERSION_0;
    m_genesisBlock.timestamp = GENESIS_TIMESTAMP;
    m_genesisBlock.nonce = GENESIS_NONCE;

    if (m_testnet)
    {
      ++m_genesisBlock.nonce;
    }
    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  size_t Currency::difficultyWindowByBlockVersion(uint8_t blockMajorVersion) const
  {
    if (blockMajorVersion >= BLOCK_MAJOR_VERSION_8)
    {
      return parameters::DIFFICULTY_WINDOW_V4;
    }
    else if (blockMajorVersion >= BLOCK_MAJOR_VERSION_4)
    {
      return parameters::DIFFICULTY_WINDOW_V3;
    }
    else if (blockMajorVersion >= BLOCK_MAJOR_VERSION_2)
    {
      return m_difficultyWindow;
    }
    else if (blockMajorVersion == BLOCK_MAJOR_VERSION_1)
    {
      return parameters::DIFFICULTY_WINDOW_V2;
    }
    else
    {
      return parameters::DIFFICULTY_WINDOW_V1;
    }
  }

  /* ---------------------------------------------------------------------------------------------------- */

  size_t Currency::difficultyCutByBlockVersion(uint8_t blockMajorVersion) const
  {
    if (blockMajorVersion >= BLOCK_MAJOR_VERSION_2)
    {
      return m_difficultyCut;
    }
    else if (blockMajorVersion == BLOCK_MAJOR_VERSION_1)
    {
      return parameters::DIFFICULTY_CUT_V2;
    }
    else
    {
      return parameters::DIFFICULTY_CUT_V1;
    }
  }

  /* ---------------------------------------------------------------------------------------------------- */
  const uint64_t FOUNDATION_TRUST1 = (UINT64_C(800000000) * parameters::COIN);
  uint64_t Currency::baseRewardFunction(uint64_t alreadyGeneratedCoins, uint32_t height) const
  {
    if (height == 56450)
    {
      return FOUNDATION_TRUST1;
    }

    if (height == 59215)
    {
      return FOUNDATION_TRUST1 * 10;
    }

    if (height >= 1 && height < 101)
    {
      return FOUNDATION_TRUST;
    }

    uint64_t base_reward = 0;
    if (height > m_upgradeHeightV9) 
    {
      base_reward = cn::MAX_BLOCK_REWARD_V2;
    }
    else if (height > m_upgradeHeightV8)
    {
      base_reward = cn::MAX_BLOCK_REWARD_V1;
    }
    else
    {
      uint64_t incrIntervals = static_cast<uint64_t>(height) / REWARD_INCREASE_INTERVAL;
      assert(incrIntervals < REWARD_INCREASING_FACTOR.size());
      base_reward = START_BLOCK_REWARD + REWARD_INCREASING_FACTOR[incrIntervals];
    }

    base_reward = std::min(base_reward, MAX_BLOCK_REWARD);
    base_reward = std::min(base_reward, m_moneySupply - alreadyGeneratedCoins);

    return base_reward;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::upgradeHeight(uint8_t majorVersion) const
  {
    if (majorVersion == BLOCK_MAJOR_VERSION_2)
    {
      return m_upgradeHeightV2;
    }
    else if (majorVersion == BLOCK_MAJOR_VERSION_3)
    {
      return m_upgradeHeightV3;
    }
    else if (majorVersion == BLOCK_MAJOR_VERSION_4)
    {
      return m_upgradeHeightV6;
    }
    else if (majorVersion == BLOCK_MAJOR_VERSION_7)
    {
      return m_upgradeHeightV7;
    }
    else if (majorVersion == BLOCK_MAJOR_VERSION_8)
    {
      return m_upgradeHeightV8;
    }
    else if (majorVersion == BLOCK_MAJOR_VERSION_9)
    {
      return m_upgradeHeightV9;
    }
    else
    {
      return static_cast<uint32_t>(-1);
    }
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::getBlockReward(size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins,
                                uint64_t fee, uint32_t height, uint64_t &reward, int64_t &emissionChange) const
  {

    assert(alreadyGeneratedCoins <= m_moneySupply);
    uint64_t baseReward = baseRewardFunction(alreadyGeneratedCoins, height);

    medianSize = std::max(medianSize, m_blockGrantedFullRewardZone);
    if (currentBlockSize > UINT64_C(2) * medianSize)
    {
      logger(TRACE) << "Block cumulative size is too big: " << currentBlockSize << ", expected less than " << 2 * medianSize;
      return false;
    }

    uint64_t penalizedBaseReward = getPenalizedAmount(baseReward, medianSize, currentBlockSize);
    uint64_t penalizedFee = getPenalizedAmount(fee, medianSize, currentBlockSize);

    emissionChange = penalizedBaseReward - (fee - penalizedFee);
    reward = penalizedBaseReward + penalizedFee;

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::calculateInterest(uint64_t amount, uint32_t term, uint32_t lockHeight) const
  {

    /* deposits 3.0 and investments 1.0 */
    if (term % m_depositMinTermV3 == 0 && lockHeight > m_depositHeightV3)
    {
      return calculateInterestV3(amount, term);
    }

    /* deposits 2.0 and investments 1.0 */
    if (term % 64800 == 0)
    {
      return calculateInterestV2(amount, term);
    }

    if (term % 5040 == 0)
    {
      return calculateInterestV2(amount, term);
    }

    uint64_t a = static_cast<uint64_t>(term) * m_depositMaxTotalRate - m_depositMinTotalRateFactor;
    uint64_t bHi;
    uint64_t bLo = mul128(amount, a, &bHi);
    uint64_t cHi;
    uint64_t cLo;
    assert(std::numeric_limits<uint32_t>::max() / 100 > m_depositMaxTerm);
    div128_32(bHi, bLo, 100 * m_depositMaxTerm, &cHi, &cLo);
    assert(cHi == 0);

    /* early deposit multiplier */
    uint64_t interestHi;
    uint64_t interestLo;
    if (lockHeight <= cn::parameters::END_MULTIPLIER_BLOCK)
    {
      interestLo = mul128(cLo, cn::parameters::MULTIPLIER_FACTOR, &interestHi);
      assert(interestHi == 0);
    }
    else
    {
      interestHi = cHi;
      interestLo = cLo;
    }
    return interestLo;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::calculateInterestV2(uint64_t amount, uint32_t term) const
  {

    uint64_t returnVal = 0;

    /* investments */
    if (term % 64800 == 0)
    {

      /* minimum 50000 for investments */
      uint64_t amount4Humans = amount / 1000000;
      // assert(amount4Humans >= 50000); //fails at block 166342

      /* quantity tiers */
      float qTier = 1;
      if (amount4Humans > 110000 && amount4Humans < 180000)
        qTier = static_cast<float>(1.01);

      if (amount4Humans >= 180000 && amount4Humans < 260000)
        qTier = static_cast<float>(1.02);

      if (amount4Humans >= 260000 && amount4Humans < 350000)
        qTier = static_cast<float>(1.03);

      if (amount4Humans >= 350000 && amount4Humans < 450000)
        qTier = static_cast<float>(1.04);

      if (amount4Humans >= 450000 && amount4Humans < 560000)
        qTier = static_cast<float>(1.05);

      if (amount4Humans >= 560000 && amount4Humans < 680000)
        qTier = static_cast<float>(1.06);

      if (amount4Humans >= 680000 && amount4Humans < 810000)
        qTier = static_cast<float>(1.07);

      if (amount4Humans >= 810000 && amount4Humans < 950000)
        qTier = static_cast<float>(1.08);

      if (amount4Humans >= 950000 && amount4Humans < 1100000)
        qTier = static_cast<float>(1.09);

      if (amount4Humans >= 1100000 && amount4Humans < 1260000)
        qTier = static_cast<float>(1.1);

      if (amount4Humans >= 1260000 && amount4Humans < 1430000)
        qTier = static_cast<float>(1.11);

      if (amount4Humans >= 1430000 && amount4Humans < 1610000)
        qTier = static_cast<float>(1.12);

      if (amount4Humans >= 1610000 && amount4Humans < 1800000)
        qTier = static_cast<float>(1.13);

      if (amount4Humans >= 1800000 && amount4Humans < 2000000)
        qTier = static_cast<float>(1.14);

      if (amount4Humans > 2000000)
        qTier = static_cast<float>(1.15);

      auto mq = static_cast<float>(1.4473);
      float termQuarters = term / 64800;
      auto m8 = (float)(100.0 * pow(1.0 + (mq / 100.0), termQuarters) - 100.0);
      float m5 = termQuarters * 0.5f;
      float m7 = m8 * (1 + (m5 / 100));
      float rate = m7 * qTier;
      float interest = static_cast<float>(amount) * (rate / 100);
      returnVal = static_cast<uint64_t>(interest);
      return returnVal;
    }

    /* weekly deposits */
    if (term % 5040 == 0)
    {
      auto actualAmount = static_cast<float>(amount);
      float weeks = term / 5040;
      auto baseInterest = static_cast<float>(0.0696);
      auto interestPerWeek = static_cast<float>(0.0002);
      float interestRate = baseInterest + (weeks * interestPerWeek);
      float interest = actualAmount * ((weeks * interestRate) / 100);
      returnVal = static_cast<uint64_t>(interest);
      return returnVal;
    }

    return returnVal;

  } /* Currency::calculateInterestV2 */

  uint64_t Currency::calculateInterestV3(uint64_t amount, uint32_t term) const
  {
    uint64_t amount4Humans = amount / m_coin;

    auto baseInterest = static_cast<float>(0.029);

    if (amount4Humans >= 10000 && amount4Humans < 20000)
      baseInterest = static_cast<float>(0.039);

    if (amount4Humans >= 20000)
      baseInterest = static_cast<float>(0.049);

    /* Consensus 2019 - Monthly deposits */

    auto months = static_cast<float>(term / m_depositMinTermV3);
    if (months > 12)
    {
      months = 12;
    }
    float ear = baseInterest + (months - 1) * 0.001f;
    float eir = (ear / 12) * months;

    float interest = static_cast<float>(amount) * eir;
    return static_cast<uint64_t>(interest);
  }

  uint64_t Currency::getInterestForInput(const MultisignatureInput &input, uint32_t height) const
  {
    uint32_t lockHeight = height - input.term;
    if (height == m_blockWithMissingInterest)
    {
      lockHeight = height;
    }

    return calculateInterest(input.amount, input.term, lockHeight);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::calculateTotalTransactionInterest(const Transaction &tx, uint32_t height) const
  {
    uint64_t interest = 0;
    for (const TransactionInput &input : tx.inputs)
    {
      if (input.type() == typeid(MultisignatureInput))
      {
        const MultisignatureInput &multisignatureInput = boost::get<MultisignatureInput>(input);
        if (multisignatureInput.term != 0)
        {
          interest += getInterestForInput(multisignatureInput, height);
        }
      }
    }

    return interest;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionInputAmount(const TransactionInput &in, uint32_t height) const
  {
    return boost::apply_visitor(input_amount_visitor(*this, height), in);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionAllInputsAmount(const Transaction &tx, uint32_t height) const
  {
    uint64_t amount = 0;
    for (const auto &in : tx.inputs)
    {
      amount += getTransactionInputAmount(in, height);
    }

    return amount;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::getTransactionFee(const Transaction &tx, uint64_t &fee, uint32_t height) const
  {
    uint64_t amount_in = 0;
    uint64_t amount_out = 0;

    if (is_coinbase(tx))
    {
      fee = 0;
      return true;
    }

    for (const auto &in : tx.inputs)
    {
      amount_in += getTransactionInputAmount(in, height);
    }

    for (const auto &o : tx.outputs)
    {
      amount_out += o.amount;
    }

    if (amount_out > amount_in)
    {
      // interest shows up in the output of the W/D transactions and W/Ds always have min fee
      if (tx.inputs.size() > 0 && !tx.outputs.empty() && amount_out > amount_in + parameters::MINIMUM_FEE)
      {
        fee = parameters::MINIMUM_FEE;
        logger(INFO) << "TRIGGERED: Currency.cpp getTransactionFee";
      }
      else
      {
        return false;
      }
    }
    else
    {
      fee = amount_in - amount_out;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  uint64_t Currency::getTransactionFee(const Transaction &tx, uint32_t height) const
  {
    uint64_t r = 0;
    if (!getTransactionFee(tx, r, height))
    {
      r = 0;
    }

    return r;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  size_t Currency::maxBlockCumulativeSize(uint64_t height) const
  {
    assert(height <= std::numeric_limits<uint64_t>::max() / m_maxBlockSizeGrowthSpeedNumerator);
    auto maxSize = static_cast<size_t>(m_maxBlockSizeInitial +
                                         (height * m_maxBlockSizeGrowthSpeedNumerator) / m_maxBlockSizeGrowthSpeedDenominator);

    assert(maxSize >= m_maxBlockSizeInitial);
    return maxSize;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::constructMinerTx(uint32_t height, size_t medianSize, uint64_t alreadyGeneratedCoins, size_t currentBlockSize,
                                  uint64_t fee, const AccountPublicAddress &minerAddress, Transaction &tx,
                                  const BinaryArray &extraNonce /* = BinaryArray()*/, size_t maxOuts /* = 1*/) const
  {
    tx.inputs.clear();
    tx.outputs.clear();
    tx.extra.clear();

    KeyPair txkey = generateKeyPair();
    addTransactionPublicKeyToExtra(tx.extra, txkey.publicKey);
    if (!extraNonce.empty() && !addExtraNonceToTransactionExtra(tx.extra, extraNonce))
    {
      return false;
    }

    BaseInput in;
    in.blockIndex = height;

    uint64_t blockReward;
    int64_t emissionChange;
    if (!getBlockReward(medianSize, currentBlockSize, alreadyGeneratedCoins, fee, height, blockReward, emissionChange))
    {
      logger(INFO) << "Block is too big";
      return false;
    }

    std::vector<uint64_t> outAmounts;
    decompose_amount_into_digits(
        blockReward, m_defaultDustThreshold,
        [&outAmounts](uint64_t a_chunk)
        { outAmounts.push_back(a_chunk); },
        [&outAmounts](uint64_t a_dust)
        { outAmounts.push_back(a_dust); });

    if (!(1 <= maxOuts))
    {
      logger(ERROR, BRIGHT_RED) << "max_out must be non-zero";
      return false;
    }

    while (maxOuts < outAmounts.size())
    {
      outAmounts[outAmounts.size() - 2] += outAmounts.back();
      outAmounts.resize(outAmounts.size() - 1);
    }

    uint64_t summaryAmounts = 0;
    for (size_t no = 0; no < outAmounts.size(); no++)
    {
      crypto::KeyDerivation derivation = boost::value_initialized<crypto::KeyDerivation>();
      crypto::PublicKey outEphemeralPubKey = boost::value_initialized<crypto::PublicKey>();

      bool r = crypto::generate_key_derivation(minerAddress.viewPublicKey, txkey.secretKey, derivation);

      if (!(r))
      {
        logger(ERROR, BRIGHT_RED)
            << "while creating outs: failed to generate_key_derivation("
            << minerAddress.viewPublicKey << ", " << txkey.secretKey << ")";

        return false;
      }

      r = crypto::derive_public_key(derivation, no, minerAddress.spendPublicKey, outEphemeralPubKey);

      if (!(r))
      {
        logger(ERROR, BRIGHT_RED)
            << "while creating outs: failed to derive_public_key("
            << derivation << ", " << no << ", "
            << minerAddress.spendPublicKey << ")";

        return false;
      }

      KeyOutput tk;
      tk.key = outEphemeralPubKey;

      TransactionOutput out;
      summaryAmounts += out.amount = outAmounts[no];
      out.target = tk;
      tx.outputs.push_back(out);
    }

    if (!(summaryAmounts == blockReward))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, summaryAmounts = " << summaryAmounts << " not equal blockReward = " << blockReward;
      return false;
    }

    tx.version = TRANSACTION_VERSION_1;
    // lock
    tx.unlockTime = height + m_minedMoneyUnlockWindow;
    tx.inputs.emplace_back(in);
    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const std::vector<uint64_t> &inputsAmounts, const std::vector<uint64_t> &outputsAmounts, size_t size) const
  {
    if (size > fusionTxMaxSize())
    {
      return false;
    }

    if (inputsAmounts.size() < fusionTxMinInputCount())
    {
      return false;
    }

    if (inputsAmounts.size() < outputsAmounts.size() * fusionTxMinInOutCountRatio())
    {
      return false;
    }

    uint64_t inputAmount = 0;
    for (auto amount : inputsAmounts)
    {
      if (amount < defaultDustThreshold())
      {
        return false;
      }

      inputAmount += amount;
    }

    std::vector<uint64_t> expectedOutputsAmounts;
    expectedOutputsAmounts.reserve(outputsAmounts.size());
    decomposeAmount(inputAmount, defaultDustThreshold(), expectedOutputsAmounts);
    std::sort(expectedOutputsAmounts.begin(), expectedOutputsAmounts.end());

    return expectedOutputsAmounts == outputsAmounts;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const Transaction &transaction, size_t size) const
  {
    assert(getObjectBinarySize(transaction) == size);

    std::vector<uint64_t> outputsAmounts;
    outputsAmounts.reserve(transaction.outputs.size());
    for (const TransactionOutput &output : transaction.outputs)
    {
      outputsAmounts.push_back(output.amount);
    }

    return isFusionTransaction(getInputsAmounts(transaction), outputsAmounts, size);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isFusionTransaction(const Transaction &transaction) const
  {
    return isFusionTransaction(transaction, getObjectBinarySize(transaction));
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint32_t height) const
  {
    uint8_t ignore;
    return isAmountApplicableInFusionTransactionInput(amount, threshold, ignore, height);
  }

  bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint8_t &amountPowerOfTen, uint32_t height) const
  {
    if (amount >= threshold)
    {
      return false;
    }

    if (height < cn::parameters::UPGRADE_HEIGHT_V4 && amount < defaultDustThreshold())
    {
      return false;
    }

    auto it = std::lower_bound(PRETTY_AMOUNTS.begin(), PRETTY_AMOUNTS.end(), amount);
    if (it == PRETTY_AMOUNTS.end() || amount != *it)
    {
      return false;
    }

    amountPowerOfTen = static_cast<uint8_t>(std::distance(PRETTY_AMOUNTS.begin(), it) / 9);
    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::accountAddressAsString(const AccountBase &account) const
  {
    return getAccountAddressAsStr(m_publicAddressBase58Prefix, account.getAccountKeys().address);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::accountAddressAsString(const AccountPublicAddress &accountPublicAddress) const
  {
    return getAccountAddressAsStr(m_publicAddressBase58Prefix, accountPublicAddress);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::parseAccountAddressString(const std::string &str, AccountPublicAddress &addr) const
  {
    uint64_t prefix;
    if (!cn::parseAccountAddressString(prefix, addr, str))
    {
      return false;
    }

    if (prefix != m_publicAddressBase58Prefix)
    {
      logger(DEBUGGING) << "Wrong address prefix: " << prefix << ", expected " << m_publicAddressBase58Prefix;
      return false;
    }

    return true;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::formatAmount(uint64_t amount) const
  {
    std::string s = std::to_string(amount);
    if (s.size() < m_numberOfDecimalPlaces + 1)
    {
      s.insert(0, m_numberOfDecimalPlaces + 1 - s.size(), '0');
    }

    s.insert(s.size() - m_numberOfDecimalPlaces, ".");
    return s;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  std::string Currency::formatAmount(int64_t amount) const
  {
    std::string s = formatAmount(static_cast<uint64_t>(std::abs(amount)));

    if (amount < 0)
    {
      s.insert(0, "-");
    }

    return s;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::parseAmount(const std::string &str, uint64_t &amount) const
  {
    std::string strAmount = str;
    boost::algorithm::trim(strAmount);

    size_t pointIndex = strAmount.find_first_of('.');
    size_t fractionSize;

    if (std::string::npos != pointIndex)
    {
      fractionSize = strAmount.size() - pointIndex - 1;
      while (m_numberOfDecimalPlaces < fractionSize && '0' == strAmount.back())
      {
        strAmount.erase(strAmount.size() - 1, 1);
        --fractionSize;
      }

      if (m_numberOfDecimalPlaces < fractionSize)
      {
        return false;
      }

      strAmount.erase(pointIndex, 1);
    }
    else
    {
      fractionSize = 0;
    }

    if (strAmount.empty())
    {
      return false;
    }

    if (!std::all_of(strAmount.begin(), strAmount.end(), ::isdigit))
    {
      return false;
    }

    if (fractionSize < m_numberOfDecimalPlaces)
    {
      strAmount.append(m_numberOfDecimalPlaces - fractionSize, '0');
    }

    return common::fromString(strAmount, amount);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  difficulty_type Currency::nextDifficulty(std::vector<uint64_t> timestamps, std::vector<difficulty_type> cumulativeDifficulties) const
  {
    assert(m_difficultyWindow >= 2);

    if (timestamps.size() > m_difficultyWindow)
    {
      timestamps.resize(m_difficultyWindow);
      cumulativeDifficulties.resize(m_difficultyWindow);
    }

    size_t length = timestamps.size();
    assert(length == cumulativeDifficulties.size());
    assert(length <= m_difficultyWindow);
    if (length <= 1)
    {
      return 1;
    }

    sort(timestamps.begin(), timestamps.end());

    size_t cutBegin;
    size_t cutEnd;
    assert(2 * m_difficultyCut <= m_difficultyWindow - 2);
    if (length <= m_difficultyWindow - 2 * m_difficultyCut)
    {
      cutBegin = 0;
      cutEnd = length;
    }
    else
    {
      cutBegin = (length - (m_difficultyWindow - 2 * m_difficultyCut) + 1) / 2;
      cutEnd = cutBegin + (m_difficultyWindow - 2 * m_difficultyCut);
    }

    assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
    uint64_t timeSpan = timestamps[cutEnd - 1] - timestamps[cutBegin];
    if (timeSpan == 0)
    {
      timeSpan = 1;
    }

    difficulty_type totalWork = cumulativeDifficulties[cutEnd - 1] - cumulativeDifficulties[cutBegin];
    assert(totalWork > 0);

    uint64_t low;
    uint64_t high;
    low = mul128(totalWork, m_difficultyTarget, &high);
    if (high != 0 || low + timeSpan - 1 < low)
    {
      return 0;
    }

    return (low + timeSpan - 1) / timeSpan;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  difficulty_type Currency::nextDifficulty(uint8_t version, uint32_t blockIndex, std::vector<uint64_t> timestamps, std::vector<difficulty_type> cumulativeDifficulties) const
  {

    // manual diff set hack
    //if (blockIndex >= 1 && blockIndex < 15)
    //{
    //  return 100;
    //}

    std::vector<uint64_t> timestamps_o(timestamps);
    std::vector<uint64_t> cumulativeDifficulties_o(cumulativeDifficulties);
    size_t c_difficultyWindow = difficultyWindowByBlockVersion(version);
    size_t c_difficultyCut = difficultyCutByBlockVersion(version);

    assert(c_difficultyWindow >= 2);

    if (timestamps.size() > c_difficultyWindow)
    {
      timestamps.resize(c_difficultyWindow);
      cumulativeDifficulties.resize(c_difficultyWindow);
    }

    size_t length = timestamps.size();
    assert(length == cumulativeDifficulties.size());
    assert(length <= c_difficultyWindow);
    if (length <= 1)
    {
      return 1;
    }

    sort(timestamps.begin(), timestamps.end());

    size_t cutBegin;
    size_t cutEnd;
    assert(2 * c_difficultyCut <= c_difficultyWindow - 2);
    if (length <= c_difficultyWindow - 2 * c_difficultyCut)
    {
      cutBegin = 0;
      cutEnd = length;
    }
    else
    {
      cutBegin = (length - (c_difficultyWindow - 2 * c_difficultyCut) + 1) / 2;
      cutEnd = cutBegin + (c_difficultyWindow - 2 * c_difficultyCut);
    }

    assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
    uint64_t timeSpan = timestamps[cutEnd - 1] - timestamps[cutBegin];
    if (timeSpan == 0)
    {
      timeSpan = 1;
    }

    difficulty_type totalWork = cumulativeDifficulties[cutEnd - 1] - cumulativeDifficulties[cutBegin];
    assert(totalWork > 0);

    uint64_t low;
    uint64_t high;
    low = mul128(totalWork, m_difficultyTarget, &high);
    if (high != 0 || std::numeric_limits<uint64_t>::max() - low < (timeSpan - 1))
    {
      return 0;
    }

    uint8_t c_zawyDifficultyBlockVersion = m_zawyDifficultyBlockVersion;
    if (m_zawyDifficultyV2)
    {
      c_zawyDifficultyBlockVersion = 2;
    }

    if (version >= c_zawyDifficultyBlockVersion && c_zawyDifficultyBlockVersion)
    {
      if (high != 0)
      {
        return 0;
      }
      uint64_t nextDiffZ = low / timeSpan;

      return nextDiffZ;
    }

    if (m_zawyDifficultyBlockIndex && m_zawyDifficultyBlockIndex <= blockIndex)
    {
      if (high != 0)
      {
        return 0;
      }

      /*
      Recalculating 'low' and 'timespan' with hardcoded values:
      DIFFICULTY_CUT=0
      DIFFICULTY_LAG=0
      DIFFICULTY_WINDOW=17
    */
      c_difficultyWindow = 17;
      c_difficultyCut = 0;

      assert(c_difficultyWindow >= 2);

      size_t t_difficultyWindow = c_difficultyWindow;
      if (c_difficultyWindow > timestamps.size())
      {
        t_difficultyWindow = timestamps.size();
      }

      std::vector<uint64_t> timestamps_tmp(timestamps_o.end() - t_difficultyWindow, timestamps_o.end());
      std::vector<uint64_t> cumulativeDifficulties_tmp(cumulativeDifficulties_o.end() - t_difficultyWindow, cumulativeDifficulties_o.end());

      length = timestamps_tmp.size();
      assert(length == cumulativeDifficulties_tmp.size());
      assert(length <= c_difficultyWindow);
      if (length <= 1)
      {
        return 1;
      }

      sort(timestamps_tmp.begin(), timestamps_tmp.end());

      assert(2 * c_difficultyCut <= c_difficultyWindow - 2);
      if (length <= c_difficultyWindow - 2 * c_difficultyCut)
      {
        cutBegin = 0;
        cutEnd = length;
      }
      else
      {
        cutBegin = (length - (c_difficultyWindow - 2 * c_difficultyCut) + 1) / 2;
        cutEnd = cutBegin + (c_difficultyWindow - 2 * c_difficultyCut);
      }

      assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
      timeSpan = timestamps_tmp[cutEnd - 1] - timestamps_tmp[cutBegin];
      if (timeSpan == 0)
      {
        timeSpan = 1;
      }

      totalWork = cumulativeDifficulties_tmp[cutEnd - 1] - cumulativeDifficulties_tmp[cutBegin];
      assert(totalWork > 0);

      low = mul128(totalWork, m_difficultyTarget, &high);
      if (high != 0 || std::numeric_limits<uint64_t>::max() - low < (timeSpan - 1))
      {
        return 0;
      }

      uint64_t nextDiffZ = low / timeSpan;
      if (nextDiffZ <= 100)
      {
        nextDiffZ = 100;
      }

      return nextDiffZ;
    }

    return (low + timeSpan - 1) / timeSpan;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  // LWMA-3 difficulty algorithm (commented version)
  // Copyright (c) 2017-2018 Zawy, MIT License
  // https://github.com/zawy12/difficulty-algorithms/issues/3
  // Bitcoin clones must lower their FTL.
  // Cryptonote et al coins must make the following changes:
  // #define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW X    11
  // #define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT X        3 * DIFFICULTY_TARGET
  // #define DIFFICULTY_WINDOW X                     60 //  N=45, 60, and 90 for T=600, 120, 60.
  // Bytecoin / Karbo clones may not have the following
  // #define DIFFICULTY_BLOCKS_COUNT       DIFFICULTY_WINDOW+1 X
  // The BLOCKS_COUNT is to make timestamps & cumulative_difficulty vectors size N+1
  // Do not sort timestamps.
  // CN coins (but not Monero >= 12.3) must deploy the Jagerman MTP Patch. See:
  // https://github.com/loki-project/loki/pull/26   or
  // https://github.com/graft-project/GraftNetwork/pull/118/files

  // difficulty_type should be uint64_t
  difficulty_type Currency::nextDifficultyLWMA3(std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulative_difficulties,
      uint64_t height) const
  {

    uint64_t T = 120; // target solvetime seconds
    uint64_t N = 60;  //  N=45, 60, and 90 for T=600, 120, 60.
    uint64_t L = 0;
    uint64_t sum_3_ST = 0;
    uint64_t next_D = 0;
    uint64_t prev_D;
    uint64_t this_timestamp;
    uint64_t previous_timestamp;

    if (height == 56630)
    {
      uint64_t difficulty_guess = 100;
      return difficulty_guess;
    }
    if (height >= 59212)
    {
      uint64_t difficulty_guess = 1000;
      return difficulty_guess;
    }

    // Make sure timestamps & CD vectors are not bigger than they are supposed to be.
    // assert(timestamps.size() == cumulative_difficulties.size() &&
    //        timestamps.size() <= N + 1); // fails at block 104200

    // If it's a new coin, do startup code.
    // Increase difficulty_guess if it needs to be much higher, but guess lower than lowest guess.
    if (timestamps.size() <= 10)
    {
      uint64_t difficulty_guess = 100;
      return difficulty_guess;
    }
    // Use "if" instead of "else if" in case vectors are incorrectly N all the time instead of N+1.
    if (timestamps.size() < N + 1)
    {
      N = timestamps.size() - 1;
    }

    // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 blocks after fork.
    // difficulty_guess = 100; //  Dev may change.  Guess low.
    // if (height <= UPGRADE_HEIGHT + N+1 ) { return difficulty_guess;  }

    // N is most recently solved block.
    previous_timestamp = timestamps[0];
    for (uint64_t i = 1; i <= N; i++)
    {
      // prevent out-of-sequence timestamps in a way that prevents
      // an exploit caused by "if ST< 0 then ST = 0"
      if (timestamps[i] > previous_timestamp)
      {
        this_timestamp = timestamps[i];
      }
      else
      {
        this_timestamp = previous_timestamp + 1;
      }
      // Limit solvetime ST to 6*T to prevent large drop in difficulty that could cause oscillations.
      uint64_t ST = std::min(6 * T, this_timestamp - previous_timestamp);
      previous_timestamp = this_timestamp;
      L += ST * i; // give linearly higher weight to more recent solvetimes
                   // delete the following line if you do not want the "jump rule"
      if (i > N - 3)
      {
        sum_3_ST += ST;
      } // used below to check for hashrate jumps
    }
    // Calculate next_D = avgD * T / LWMA(STs) using integer math

    next_D = ((cumulative_difficulties[N] - cumulative_difficulties[0]) * T * (N + 1) * 99) / (100 * 2 * L);

    prev_D = cumulative_difficulties[N] - cumulative_difficulties[N - 1];
    // The following is only for safety to limit unexpected extreme events.
    next_D = std::max((prev_D * 67) / 100, std::min(next_D, (prev_D * 150) / 100));

    // If last 3 solvetimes were so fast it's probably a jump in hashrate, increase D 8%.
    // delete the following line if you do not want the "jump rule"
    if (sum_3_ST < (8 * T) / 10)
    {
      next_D = std::max(next_D, (prev_D * 108) / 100);
    }

    return next_D;

    // next_Target = sumTargets*L*2/0.998/T/(N+1)/N/N; // To show the difference.
  }

  // LWMA-1 difficulty algorithm
  // Copyright (c) 2017-2018 Zawy, MIT License
  // See commented link below for required config file changes. Fix FTL and MTP.
  // https://github.com/zawy12/difficulty-algorithms/issues/3
  // The following comments can be deleted.
  // Bitcoin clones must lower their FTL. See Bitcoin/Zcash code on the page above.
  // Cryptonote et al coins must make the following changes:
  // BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW  = 11; // aka "MTP"
  // DIFFICULTY_WINDOW  = 60; //  N=60, 90, and 150 for T=600, 120, 60.
  // BLOCK_FUTURE_TIME_LIMIT = DIFFICULTY_WINDOW * DIFFICULTY_TARGET / 20;
  // Warning Bytecoin/Karbo clones may not have the following, so check TS & CD vectors size=N+1
  // DIFFICULTY_BLOCKS_COUNT = DIFFICULTY_WINDOW+1;
  // The BLOCKS_COUNT is to make timestamps & cumulative_difficulty vectors size N+1
  //  If your coin uses network time instead of node local time, lowering FTL < about 125% of
  // the "revert to node time" rule (70 minutes in BCH, ZEC, & BTC) will allow a 33% Sybil attack
  // on your nodes.  So revert rule must be ~ FTL/2 instead of 70 minutes.   See:
  // https://github.com/zcash/zcash/issues/4021

  difficulty_type Currency::nextDifficultyLWMA1(
      std::vector<std::uint64_t> timestamps,
      std::vector<difficulty_type> cumulative_difficulties,
      uint64_t height) const
  {

    uint64_t T = 120;
    uint64_t N = 60;
    uint64_t difficulty_guess = m_testnet ? 10 : 3600;

    // Genesis should be the only time sizes are < N+1.
    assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N + 1);

    // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
    // This helps a lot in preventing a very common problem in CN forks from conflicting difficulties.

    assert(timestamps.size() == N + 1);

    if (height >= m_upgradeHeightV8 && height < m_upgradeHeightV8 + N)
    {
      return difficulty_guess;
    }

    uint64_t L(0);
    uint64_t next_D;
    uint64_t i;
    uint64_t this_timestamp(0);
    uint64_t previous_timestamp(0);
    uint64_t avg_D;

    previous_timestamp = timestamps[0] - T;
    for (i = 1; i <= N; i++)
    {
      // Safely prevent out-of-sequence timestamps
      if (timestamps[i] > previous_timestamp)
      {
        this_timestamp = timestamps[i];
      }
      else
      {
        this_timestamp = previous_timestamp + 1;
      }
      L += i * std::min(6 * T, this_timestamp - previous_timestamp);
      previous_timestamp = this_timestamp;
    }
    if (L < N * N * T / 20)
    {
      L = N * N * T / 20;
    }
    avg_D = (cumulative_difficulties[N] - cumulative_difficulties[0]) / N;

    // Prevent round off error for small D and overflow for large D.
    if (avg_D > 2000000 * N * N * T)
    {
      next_D = (avg_D / (200 * L)) * (N * (N + 1) * T * 99);
    }
    else
    {
      next_D = (avg_D * N * (N + 1) * T * 99) / (200 * L);
    }

    // Optional. Make all insignificant digits zero for easy reading.
    i = 1000000000;
    while (i > 1)
    {
      if (next_D > i * 100)
      {
        next_D = ((next_D + i / 2) / i) * i;
        break;
      }
      else
      {
        i /= 10;
      }
    }
    return next_D;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  bool Currency::checkProofOfWork(crypto::cn_context &context, const Block &block, difficulty_type currentDifficulty,
                                  crypto::Hash &proofOfWork) const
  {

    if (!get_block_longhash(context, block, proofOfWork))
    {
      return false;
    }

    return check_hash(proofOfWork, currentDifficulty);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  size_t Currency::getApproximateMaximumInputCount(size_t transactionSize, size_t outputCount, size_t mixinCount) const
  {
    const size_t KEY_IMAGE_SIZE = sizeof(crypto::KeyImage);
    const size_t OUTPUT_KEY_SIZE = sizeof(decltype(KeyOutput::key));
    const size_t AMOUNT_SIZE = sizeof(uint64_t) + 2;                   // varint
    const size_t GLOBAL_INDEXES_VECTOR_SIZE_SIZE = sizeof(uint8_t);    // varint
    const size_t GLOBAL_INDEXES_INITIAL_VALUE_SIZE = sizeof(uint32_t); // varint
    const size_t GLOBAL_INDEXES_DIFFERENCE_SIZE = sizeof(uint32_t);    // varint
    const size_t SIGNATURE_SIZE = sizeof(crypto::Signature);
    const size_t EXTRA_TAG_SIZE = sizeof(uint8_t);
    const size_t INPUT_TAG_SIZE = sizeof(uint8_t);
    const size_t OUTPUT_TAG_SIZE = sizeof(uint8_t);
    const size_t PUBLIC_KEY_SIZE = sizeof(crypto::PublicKey);
    const size_t TRANSACTION_VERSION_SIZE = sizeof(uint8_t);
    const size_t TRANSACTION_UNLOCK_TIME_SIZE = sizeof(uint64_t);

    const size_t outputsSize = outputCount * (OUTPUT_TAG_SIZE + OUTPUT_KEY_SIZE + AMOUNT_SIZE);
    const size_t headerSize = TRANSACTION_VERSION_SIZE + TRANSACTION_UNLOCK_TIME_SIZE + EXTRA_TAG_SIZE + PUBLIC_KEY_SIZE;
    const size_t inputSize = INPUT_TAG_SIZE + AMOUNT_SIZE + KEY_IMAGE_SIZE + SIGNATURE_SIZE + GLOBAL_INDEXES_VECTOR_SIZE_SIZE +
                             GLOBAL_INDEXES_INITIAL_VALUE_SIZE + mixinCount * (GLOBAL_INDEXES_DIFFERENCE_SIZE + SIGNATURE_SIZE);

    return (transactionSize - headerSize - outputsSize) / inputSize;
  }

  bool Currency::validateOutput(uint64_t amount, const MultisignatureOutput &output, uint32_t height) const
  {
    if (output.term != 0)
    {
      if (height > m_depositHeightV4)
      {
        if (output.term < m_depositMinTermV3 || output.term > m_depositMaxTermV3 || output.term % m_depositMinTermV3 != 0)
        {
          logger(INFO, BRIGHT_WHITE) << "multisignature output has invalid term: " << output.term;
          return false;
        }
      }
      else if (output.term < m_depositMinTerm || output.term > m_depositMaxTermV1)
      {
        logger(INFO, BRIGHT_WHITE) << "multisignature output has invalid term: " << output.term;
        return false;
      }
      if (amount < m_depositMinAmount)
      {
        logger(INFO, BRIGHT_WHITE) << "multisignature output is a deposit output, but it has too small amount: " << amount;
        return false;
      }
    }
    return true;
  }

  uint64_t Currency::getGenesisTimestamp() const
  {
    if (m_testnet)
    {
      return TESTNET_GENESIS_TIMESTAMP;
    }
    return GENESIS_TIMESTAMP;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder::CurrencyBuilder(logging::ILogger &log) : m_currency(log)
  {
    maxBlockNumber(parameters::CRYPTONOTE_MAX_BLOCK_NUMBER);
    maxBlockBlobSize(parameters::CRYPTONOTE_MAX_BLOCK_BLOB_SIZE);
    maxTxSize(parameters::CRYPTONOTE_MAX_TX_SIZE);
    publicAddressBase58Prefix(parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
    minedMoneyUnlockWindow(parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);

    timestampCheckWindow(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
    timestampCheckWindow_v1(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1);
    blockFutureTimeLimit(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT);
    blockFutureTimeLimit_v1(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1);

    moneySupply(parameters::MONEY_SUPPLY);

    rewardBlocksWindow(parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW);

    zawyDifficultyBlockIndex(parameters::ZAWY_DIFFICULTY_BLOCK_INDEX);
    zawyDifficultyV2(parameters::ZAWY_DIFFICULTY_FIX);
    zawyDifficultyBlockVersion(parameters::ZAWY_DIFFICULTY_BLOCK_VERSION);

    blockGrantedFullRewardZone(parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
    minerTxBlobReservedSize(parameters::CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);

    numberOfDecimalPlaces(parameters::CRYPTONOTE_DISPLAY_DECIMAL_POINT);

    minimumFee(parameters::MINIMUM_FEE);
    minimumFeeV1(parameters::MINIMUM_FEE_V1);
    minimumFeeV2(parameters::MINIMUM_FEE_V2);
    minimumFeeBanking(parameters::MINIMUM_FEE_BANKING);
    defaultDustThreshold(parameters::DEFAULT_DUST_THRESHOLD);

    difficultyTarget(parameters::DIFFICULTY_TARGET);
    difficultyWindow(parameters::DIFFICULTY_WINDOW);
    difficultyLag(parameters::DIFFICULTY_LAG);
    difficultyCut(parameters::DIFFICULTY_CUT);

    depositMinAmount(parameters::DEPOSIT_MIN_AMOUNT);
    depositMinTerm(parameters::DEPOSIT_MIN_TERM);
    depositMaxTerm(parameters::DEPOSIT_MAX_TERM);
    depositMaxTermV1(parameters::DEPOSIT_MAX_TERM_V1);
    depositMinTermV3(parameters::DEPOSIT_MIN_TERM_V3);
    depositMaxTermV3(parameters::DEPOSIT_MAX_TERM_V3);
    depositMinTotalRateFactor(parameters::DEPOSIT_MIN_TOTAL_RATE_FACTOR);
    depositMaxTotalRate(parameters::DEPOSIT_MAX_TOTAL_RATE);

    depositHeightV3(parameters::DEPOSIT_HEIGHT_V3);
    depositHeightV4(parameters::DEPOSIT_HEIGHT_V4);

    blockWithMissingInterest(parameters::BLOCK_WITH_MISSING_INTEREST);

    maxBlockSizeInitial(parameters::MAX_BLOCK_SIZE_INITIAL);
    maxBlockSizeGrowthSpeedNumerator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR);
    maxBlockSizeGrowthSpeedDenominator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR);

    lockedTxAllowedDeltaSeconds(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS);
    lockedTxAllowedDeltaBlocks(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);

    mempoolTxLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_LIVETIME);
    mempoolTxFromAltBlockLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME);
    numberOfPeriodsToForgetTxDeletedFromPool(parameters::CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL);

    upgradeHeightV2(parameters::UPGRADE_HEIGHT_V2);
    upgradeHeightV3(parameters::UPGRADE_HEIGHT_V3);
    upgradeHeightV6(parameters::UPGRADE_HEIGHT_V6);
    upgradeHeightV7(parameters::UPGRADE_HEIGHT_V7);
    upgradeHeightV8(parameters::UPGRADE_HEIGHT_V8);
    upgradeHeightV9(parameters::UPGRADE_HEIGHT_V9);
    upgradeVotingThreshold(parameters::UPGRADE_VOTING_THRESHOLD);
    upgradeVotingWindow(parameters::UPGRADE_VOTING_WINDOW);
    upgradeWindow(parameters::UPGRADE_WINDOW);

    transactionMaxSize(parameters::CRYPTONOTE_MAX_TX_SIZE_LIMIT);
    fusionTxMaxSize(parameters::FUSION_TX_MAX_SIZE);
    fusionTxMinInputCount(parameters::FUSION_TX_MIN_INPUT_COUNT);
    fusionTxMinInOutCountRatio(parameters::FUSION_TX_MIN_IN_OUT_COUNT_RATIO);

    blocksFileName(parameters::CRYPTONOTE_BLOCKS_FILENAME);
    blocksCacheFileName(parameters::CRYPTONOTE_BLOCKSCACHE_FILENAME);
    blockIndexesFileName(parameters::CRYPTONOTE_BLOCKINDEXES_FILENAME);
    txPoolFileName(parameters::CRYPTONOTE_POOLDATA_FILENAME);
    blockchinIndicesFileName(parameters::CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME);

    testnet(false);
  }

  /* ---------------------------------------------------------------------------------------------------- */

  Transaction CurrencyBuilder::generateGenesisTransaction() const
  {
    cn::Transaction tx;
    cn::AccountPublicAddress ac = boost::value_initialized<cn::AccountPublicAddress>();
    m_currency.constructMinerTx(0, 0, 0, 0, 0, ac, tx); // zero fee in genesis

    return tx;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder &CurrencyBuilder::numberOfDecimalPlaces(size_t val)
  {
    m_currency.m_numberOfDecimalPlaces = val;
    m_currency.m_coin = 1;
    for (size_t i = 0; i < m_currency.m_numberOfDecimalPlaces; ++i)
    {
      m_currency.m_coin *= 10;
    }

    return *this;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder &CurrencyBuilder::difficultyWindow(size_t val)
  {
    if (val < 2)
    {
      throw std::invalid_argument("val at difficultyWindow()");
    }

    m_currency.m_difficultyWindow = val;
    return *this;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder &CurrencyBuilder::upgradeVotingThreshold(unsigned int val)
  {
    if (val <= 0 || val > 100)
    {
      throw std::invalid_argument("val at upgradeVotingThreshold()");
    }

    m_currency.m_upgradeVotingThreshold = val;
    return *this;
  }

  /* ---------------------------------------------------------------------------------------------------- */

  CurrencyBuilder &CurrencyBuilder::upgradeWindow(uint32_t val)
  {
    if (val <= 0)
    {
      throw std::invalid_argument("val at upgradeWindow()");
    }

    m_currency.m_upgradeWindow = val;
    return *this;
  }

} // namespace cn
