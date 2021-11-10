pragma ton-solidity >=0.47.0;

abstract contract ADeployUserToken {
    function deployUserToken(uint256 pubkey, address root) public view virtual returns(address addr) {}
}
