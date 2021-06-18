pragma ton-solidity >=0.35.0;

interface IAmountInput {

	function get(uint32 answerId, string prompt, uint8 decimals, uint128 min, uint128 max) external returns (uint128 value);

}

library AmountInput {

	uint256 constant ID = 0xa1d347099e29c1624c8890619daf207bde18e92df5220a54bcc6d858309ece84;
	int8 constant DEBOT_WC = -31;

	function get(uint32 answerId, string prompt, uint8 decimals, uint128 min, uint128 max) public pure {
		address addr = address.makeAddrStd(DEBOT_WC, ID);
		IAmountInput(addr).get(answerId, prompt, decimals, min, max);
	}

}

contract AmountInputABI is IAmountInput {

	function get(uint32 answerId, string prompt, uint8 decimals, uint128 min, uint128 max) external override returns (uint128 value) {}

}