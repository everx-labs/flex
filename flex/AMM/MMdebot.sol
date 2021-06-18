pragma ton-solidity >=0.35.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
import "debots/Debot.sol";
import "debots/Terminal.sol";
import "debots/AddressInput.sol";
import "debots/ConfirmInput.sol";

interface MarketMaker {
    function UpdateInfo(StockTonCfg value0, uint128 value1, uint8 value2, TvmCell value3) external;
    function AddTip3Wallet (T3WDetails value1, address value2) external;
    function AddPubKey (uint256 value0) external;
    function setPriceCode (TvmCell value0) external;
    function getInfo2() external returns(uint128 m_stockMinAmount, uint128 m_stockDealsLimit);
}

abstract contract ATip3Wallet {
    function getDetails() public returns (bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) {}
}

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}

abstract contract AStock {
    function getTradingPairCode() public returns(TvmCell value0) {}
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
    function getMinAmount() public returns(uint128 value0) {}
    function getDealsLimit() public returns(uint8 value0) {}
    function getTonsCfg() public returns(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
}

struct Allowance{
    address spender;
    uint128 remainingTokens;
}

struct T3WDetails {
    bytes name;
    bytes symbol;
    uint256 root_public_key;
    uint256 wallet_public_key;
    address root_address;
    address owner_address;
    TvmCell code;
    uint8 decimals;
    uint128 balance;
    LendOwnership lend_ownership;
}

struct LendOwnership{
   address owner;
   uint128 lend_balance;
   uint32 lend_finish_time;
} 

contract HelloDebot is Debot {
    address m_stockAddr;
    address _MarketMaker;
    address _notify;
    address _tip3;
    StockTonCfg m_stockTonCfg;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    TvmCell m_tradingPairCode;     
    T3WDetails m_tip3walletDetails;
    uint32 m_getT3WDetailsCallback;
    bool add = true;
    address m_tip3root;
    TvmCell m_sellPriceCode;
    uint256 m_masterPubKey;

    function start() public override {
        AddressInput.get(tvm.functionId(setStock), "Enter address of Stock");
        AddressInput.get(tvm.functionId(setMM), "Enter address of MarketMaker");
        AddressInput.get(tvm.functionId(setNotify), "Enter address of Notify");
        AddressInput.get(tvm.functionId(setTIP3), "Enter address of tip3root");
        ConfirmInput.get(tvm.functionId(UpdateInfo), "Do you want to Get Information and send to MM?");
    }

    function setTIP3(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        m_tip3root = value;
    }

    
    function AddTip3W (bool value) public {
        if (value == true){
            AddressInput.get(tvm.functionId(AddTip3), "Enter address of Tip3");
        }
    } 
    

    function submit(bool value) public {
        add = value;
    }

    function getT3WDetails(bool value) public {
        if (value == false) { return; }
        optional(uint256) none;
        ATip3Wallet(_tip3).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setT3WDetails),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
        Terminal.print(0, format("Get! {}", _tip3));
    }

    function enterPublicKey() public {
        Terminal.input(tvm.functionId(getPublicKey),"Please enter your public key",false);
    }

    function enterPublicKey1(bool value) public {
        if (value == false) { return; }
        sendTip3Wallet(true);
    }

    function getPublicKey(string value) public {
        uint res;
        bool status;
        (res, status) = stoi("0x"+value);
        if (status) {
            m_masterPubKey = res;
            sendPubKey(true);
        } else
            Terminal.input(tvm.functionId(enterPublicKey),"Wrong public key. Try again!\nPlease enter your public key",false);
    }

    function setT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.name = name;
        m_tip3walletDetails.symbol = symbol;
        m_tip3walletDetails.root_public_key = root_public_key;
        m_tip3walletDetails.wallet_public_key = wallet_public_key;
        m_tip3walletDetails.root_address = root_address;
        m_tip3walletDetails.owner_address = owner_address;
        m_tip3walletDetails.code = code;
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        Terminal.print(0, format("Got details!"));
        ConfirmInput.get(tvm.functionId(enterPublicKey1), "Do you want to sendTip3?");
    }

    function getPriceCodeHash(bool value) public {
        if (value == false) { return; }
        optional(uint256) none;
        AStock(m_stockAddr).getSellPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceCode),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3root);
    }
    
    function setPriceCode(TvmCell value0) public {
        m_sellPriceCode = value0;
        uint256 h = tvm.hash(m_sellPriceCode);
        Terminal.print(0,format("stock address {}",m_stockAddr));
        Terminal.print(0,format("tip3 root address {}",m_tip3root));
        Terminal.print(0,format("price code hash {:x}",h));
        ConfirmInput.get(tvm.functionId(sendPriceCode), "Do you want to sendPriceCode?");
    }
   
    function sendPriceCode(bool value) public {
        if (value == false) { return; }
        optional(uint256) none;
        MarketMaker(_MarketMaker).setPriceCode {
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(sended),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(m_sellPriceCode);
    }
   
    function sended() public {
        Terminal.print(0, format("Success!"));
        ConfirmInput.get(tvm.functionId(AddPubKey), "Do you want to add pubkey?");
    }

    function sended1() public {
        ConfirmInput.get(tvm.functionId(AddTip3W), "Do you want to add Tip3wallet?");    
    }

    function AddPubKey(bool value) public {
        if (value == true){ 
            enterPublicKey();
        }
        else { 
            sended1();
        }
    }

    function sendPubKey(bool value) public {
        if (value == false) {
            return;
        } 
        optional(uint256) none;
        MarketMaker(_MarketMaker).AddPubKey{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(doNothing1),
            onErrorId: tvm.functionId(ErrorB1),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(m_masterPubKey);
    }
 
    function sendTip3Wallet(bool value) public {
        if (value == false) {
            return;
        } 
        optional(uint256) none;
        MarketMaker(_MarketMaker).AddTip3Wallet{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(doNothing),
            onErrorId: tvm.functionId(ErrorB),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(m_tip3walletDetails, _tip3);
    }

    function AddTip3(address value) public {
        _tip3 = value;
        ConfirmInput.get(tvm.functionId(getT3WDetails), "Confirm?");
    }

    function ErrorB() public{
        Terminal.print(0, format("Error!"));
        ConfirmInput.get(tvm.functionId(AddTip3W), "Do you want to add Tip3wallet?");
    }

    function ErrorB1() public{
        Terminal.print(0, format("Error!"));
        sended();
    }

    function doNothing() public{
        Terminal.print(0, format("Success!"));
        ConfirmInput.get(tvm.functionId(AddTip3W), "Do you want to add Tip3wallet?");
    }

    function doNothing1() public{
        Terminal.print(0, format("Success!"));
        sended();
    }

    function setStock(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        m_stockAddr = value;
    }

    function setMM(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        _MarketMaker = value;
    }

    function setNotify(address value) public {
       _notify = value;
    }

    function UpdateInfo(bool value) public {
        if (value == false){
            return;
        }
        getStockTonsCfg();
    }
 
    function sendInfo() public {
        optional(uint256) none;
         Terminal.print(0, format("Info for MarketMaker! m_stockMinAmount {} m_stockDealsLimit {}", m_stockMinAmount, m_stockDealsLimit));
       MarketMaker(_MarketMaker).UpdateInfo{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(SetCfg2),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true, pubkey: none} (m_stockTonCfg, m_stockMinAmount, m_stockDealsLimit, m_tradingPairCode);
    }
  
    function SetCfg2() public {
        ConfirmInput.get(tvm.functionId(SetCfg3), "Do you want to continue");
    }

    function SetCfg3(bool value) public {
        if (value == false){
            return;
        }
        optional(uint256) none;
        MarketMaker(_MarketMaker).getInfo2{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(SetCfg4),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function SetCfg4(uint128 value0, uint128 value1) public {
        Terminal.print(0, format("Got from MM {} {}!", value0, value1));
        ConfirmInput.get(tvm.functionId(SetCfg5), "Do you want to continue");
    }

    function SetCfg5(bool value) public {
        if (value == false){ return;}
        Confirm();
    }

    function Confirm ()  public {
        ConfirmInput.get(tvm.functionId(getPriceCodeHash), "Do you want to getPriceCodeHash?");
    }

    function ErrorCatch() public {
        Terminal.print(0, "Sad!");
    }

    function getStockTonsCfg() public {
       Terminal.print(0,format("m_stockAddr = {}",m_stockAddr));
       optional(uint256) none;
        AStock(m_stockAddr).getTonsCfg{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockTonsCfg),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setStockTonsCfg(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) public {
        Terminal.print(0, "Got StockTonsCfg!");
        m_stockTonCfg.transfer_tip3 = transfer_tip3;
        m_stockTonCfg.return_ownership = return_ownership;
        m_stockTonCfg.trading_pair_deploy = trading_pair_deploy;
        m_stockTonCfg.order_answer = order_answer;
        m_stockTonCfg.process_queue = process_queue;
        m_stockTonCfg.send_notify = send_notify;
        optional(uint256) none;
        AStock(m_stockAddr).getMinAmount{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockMinAmount),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setStockMinAmount(uint128 value0) public {
        Terminal.print(0, "Got StockMinAmount!");
        m_stockMinAmount = value0;
        optional(uint256) none;
        AStock(m_stockAddr).getDealsLimit{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getTradingPairCode),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getTradingPairCode(uint8 value0) public {
        Terminal.print(0, "Got TradingPairCode!");
        m_stockDealsLimit = value0;
        optional(uint256) none;
        AStock(m_stockAddr).getTradingPairCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTradingPairCode),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTradingPairCode(TvmCell value0) public {
        m_tradingPairCode = value0;
        uint256 h = tvm.hash(m_tradingPairCode);
        Terminal.print(0,format("price code hash {:x}",h));
        sendInfo();
    }

    function getVersion() public override returns (string name, uint24 semver) {
        (name, semver) = ("HelloWorld DeBot", _version(0,1,0));
    }

    function _version(uint24 major, uint24 minor, uint24 fix) private pure inline returns (uint24) {
        return (major << 16) | (minor << 8) | (fix);
    }

}

