pragma ton-solidity >=0.47.0;

import "Tip3Config.sol";
/// Request to deploy exchange pair (tip3/tip3)
struct XchgPairListingRequest {
  address client_addr;     ///< Client address to send notification with the remaining funds
  uint128 client_funds;    ///< Native funds, sent by client
  address tip3_major_root; ///< Address of RootTokenContract for the major tip3 token
  address tip3_minor_root; ///< Address of RootTokenContract for the minor tip3 token
  Tip3Config major_tip3cfg;
  Tip3Config minor_tip3cfg;
  uint128 min_amount;      ///< Minimum amount of major tokens for a deal or an order
  address notify_addr;     ///< Notification address (AMM)
}

/// Request to deploy exchange pair (tip3/tip3) with public key (for getter)
struct XchgPairListingRequestWithPubkey {
  uint256 pubkey;                 ///< Request's public key
  XchgPairListingRequest request; ///< Request's details
}