/** \file
 *  \brief WrapperDeployerBroxus arguments structure (for `deploy` call)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

namespace tvm {

/// WrapperDeployerBroxus arguments structure (for `deploy` call)
struct WrapperDeployerBroxusArgs {
  Tip3Config tip3cfg; ///< Tip3 configuration for the external wallet
};

} // namespace tvm
