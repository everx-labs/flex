pragma ton-solidity >=0.47.0;

import "Tip3Config.sol";

/// Request to deploy wrapper
struct WrapperListingRequest {
  /// Client address to send notification with the remaining funds
  address client_addr;
  /// Native funds, sent by client
  uint128 client_funds;
  /// Configuration of external tip3 wallet
  Tip3Config tip3cfg;
}

/// Request to deploy wrapper with public key (for getter)
struct WrapperListingRequestWithPubkey {
  uint256 wrapper_pubkey;        ///< Wrapper's public key
  WrapperListingRequest request; ///< Request's details
}
