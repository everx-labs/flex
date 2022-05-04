pragma ton-solidity >=0.35.0;

interface IMsg {

    function sendWithKeypair(uint32 answerId, TvmCell message, uint256 pub, uint256 sec) external;
    function sendAsync(TvmCell message) external returns(uint256 id);
}

library Msg {

	uint256 constant ID = 0x475a5d1729acee4601c2a8cb67240e4da5316cc90a116e1b181d905e79401c51;
	int8 constant DEBOT_WC = -31;

	function sendWithKeypair(uint32 answerId, TvmCell message, uint256 pub, uint256 sec) public pure {
		address addr = address.makeAddrStd(DEBOT_WC, ID);
		IMsg(addr).sendWithKeypair(answerId, message, pub, sec);
	}

	function sendAsync(TvmCell message) public pure {
		address addr = address.makeAddrStd(DEBOT_WC, ID);
		IMsg(addr).sendAsync(message);
	}

}

contract MsgABI is IMsg {

    function sendWithKeypair(uint32 answerId, TvmCell message, uint256 pub, uint256 sec) public override {}
    function sendAsync(TvmCell message) public override returns(uint256 id) {}

}
