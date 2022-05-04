pragma ton-solidity >=0.47.0;

/// Tip3 token configuration
struct Tip3Config {
  string  name;            ///< Token name.
  string  symbol;          ///< Token short symbol.
  uint8   decimals;        ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint256 root_pubkey; ///< Public key of RootTokenContract for the tip3 token.
  address root_address;    ///< Address of RootTokenContract for the tip3 token.
}