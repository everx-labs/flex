/** \file
 *  \brief Calculate Wrapper's reserve wallet address.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 *  Macroses TIP3_WRAPPER_WALLET_CODE_HASH / TIP3_WRAPPER_WALLET_CODE_DEPTH
 *   must be defined in compilation (code hash and code depth of FlexWallet).
 */

#pragma once

#include "TONTokenWallet.hpp"
#include <tvm/suffixes.hpp>

#ifndef TIP3_WALLET_CODE_HASH
#error "Macros TIP3_WALLET_CODE_HASH must be defined (code hash of FlexWallet)"
#endif

#ifndef TIP3_WALLET_CODE_DEPTH
#error "Macros TIP3_WALLET_CODE_DEPTH must be defined (code depth of FlexWallet)"
#endif

namespace tvm {

/// Calculate Wrapper's reserve wallet address
__always_inline
address calc_wrapper_reserve_wallet(Tip3Config wrapper_cfg) {
  auto workchain_id = std::get<addr_std>(wrapper_cfg.root_address.val()).workchain_id;
  auto hash = calc_int_wallet_init_hash(
    wrapper_cfg.name, wrapper_cfg.symbol, wrapper_cfg.decimals,
    wrapper_cfg.root_pubkey, wrapper_cfg.root_address,
    0u256, wrapper_cfg.root_address,
    uint256(TIP3_WALLET_CODE_HASH), uint16(TIP3_WALLET_CODE_DEPTH),
    workchain_id
  );
  return address::make_std(workchain_id, hash);
}

} // namespace tvm
