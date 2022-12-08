/** \file
 *  \brief Arguments for transfer ever tokens from old WrapperEver into the new one
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

namespace tvm {

/// Arguments for transfer ever tokens from old WrapperEver into the new one
struct EverReTransferArgs {
  uint128 wallet_deploy_evers; ///< Evers to be sent to the deployable wallet.
  uint128 wallet_keep_evers;   ///< Evers to be kept in the deployable wallet.
};

} // namespace tvm
