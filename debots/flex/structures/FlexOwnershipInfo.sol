pragma ton-solidity >=0.47.0;

/// Ownership info for Flex root
struct FlexOwnershipInfo {
  uint256 deployer_pubkey;              ///< Deployer's public key
  string ownership_description;         ///< Ownership description
  optional(address) owner; ///< If Flex is managed by other contract (deployer will not be able to execute non-deploy methods)
}