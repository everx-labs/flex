/** \file
 *  \brief Tip3Creds - tip3 wallet credentials (pubkey + wallet)
 *
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Tip3 wallet credentials
struct Tip3Creds {
  uint256     pubkey;
  address_opt owner;
};

} // namespace tvm

