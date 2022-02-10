/** \file
 *  \brief PriceXchgSalt - configuration structure for PriceXchg (stored in code salt)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Price configuration data (common for prices of one pair). Stored in code salt.
struct PriceXchgSalt {
  address     flex;                 ///< Address of root flex contract (IFlex).
  address     pair;                 ///< Address of XchgPair contract.
  address     notify_addr;          ///< Notification address for AMM (IFlexNotify).
  Tip3Config  major_tip3cfg;        ///< Configuration of the major tip3 token.
  Tip3Config  minor_tip3cfg;        ///< Configuration of the minor tip3 token.
  address     major_reserve_wallet; ///< Major reserve wallet.
  address     minor_reserve_wallet; ///< Minor reserve wallet.
  EversConfig ev_cfg;               ///< Processing costs configuration of Flex in native funds (evers).
  uint128     min_amount;           ///< Minimum amount of major tokens, allowed to make a deal or an order.
  uint8       deals_limit;          ///< Limit for processed deals in one request.
  int8        workchain_id;         ///< Workchain id for the related tip3 token wallets.
};

} // namespace tvm
