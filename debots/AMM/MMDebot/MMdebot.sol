pragma ton-solidity ^0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
import "debots/Debot.sol";
import "debots/Terminal.sol";
import "debots/AddressInput.sol";
import "debots/ConfirmInput.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/UserInfo/UserInfo.sol";
import "Upgradable.sol";
import "Sdk.sol";

abstract contract MarketMaker {
    function DeployFlex() external functionID(0xbd) {}
    function AddTip3Wallet (T3WDetails value1, address value2) external functionID(0x17) {}
    function getStockDetails() external functionID(0xbbbb) returns(uint128, uint128) {}
    constructor(uint256 value0) public {}
    function UpdateInfo(StockTonCfg value0, uint128 value1, uint8 value2) external functionID(0xb) {}
    function UpdateInfo1(TvmCell value3) external functionID(0xbb) {}
    function UpdateInfo2(TvmCell value4) external functionID(0xbbb) {}
    function init(address stockaddress, uint128 step, uint128 number, address notify) external  functionID(0xd) {}
    function setCodeflex(TvmCell c) external  functionID(0xe) {}
    function setDataflex(TvmCell c) external  functionID(0x11) {}
    function setCodeW(TvmCell c) external  functionID(0x12) {}
    function setCodepool(TvmCell c) external  functionID(0x13) {}
    function setDatapool(TvmCell c) external  functionID(0x14) {}
    function setPriceCode (TvmCell value0) external functionID(0x15) {}
}

abstract contract AStockNotify {
    function addSub(address addr) external functionID(0xbaa) {}
}

struct LendOwnership{
   address lend_addr;
   uint128 lend_balance;
   uint32 lend_finish_time;
}

struct Allowance{
    address spender;
    uint128 remainingTokens;
}

abstract contract AFlexWallet {
    function getDetails() public functionID(0x15) 
        returns (
            bytes name, bytes symbol, uint8 decimals, uint128 balance, 
            uint256 root_public_key, uint256 wallet_public_key, address root_address, 
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, 
            TvmCell code, Allowance allowance, int8 workchain_id) {}
}

abstract contract TonTokenWallet {
    function requestBalance() external functionID(0xe) returns(uint128) {}
}

abstract contract AWrapper {
    function getDetails() public functionID(0xf) returns(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet) {}
    function deployEmptyWallet(uint32 _answer_id, uint256 pubkey, address internal_owner, uint128 grams) public functionID(0xc) returns(address value0) {}
    function getWalletAddress(uint256 pubkey, address owner) public functionID(0x11) returns(address value0) {}
 }

abstract contract AStock {
    function getTonsCfg() public functionID(0x10)
        returns(
            uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
            uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
    function getTradingPairCode() public functionID(0x11) returns(TvmCell value0) {}
    function getXchgPairCode() public functionID(0x12) returns(TvmCell value0) {}
    function getDealsLimit() public functionID(0x17) returns(uint8 value0) {}
    function getSellPriceCode(address tip3_addr) public functionID(0x13) returns(address value0) {}
    function getXchgPriceCode(address tip3_addr1, address tip3_addr2) public functionID(0x14) returns(address value0) {}
}

abstract contract ATradingPair {
    function getTip3Root() public functionID(0xc) returns(address value0) {}
    function getFlexAddr() public functionID(0xb) returns(address value0) {}
    function getMinAmount() public functionID(0xd) returns(uint128 value0) {}
    function getNotifyAddr() public functionID(0xe) returns(address value0) {}
}

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}

struct T3WDetails {
    bytes name;
    bytes symbol;
    uint8 decimals;
    uint256 root_public_key;
    address root_address;
} 

contract HelloDebot is Debot, Upgradable{
    address m_stockAddr;
    address _MarketMaker;
    address _notify;
    address _tip3;
    StockTonCfg m_stockTonCfg;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    TvmCell m_tradingPairCode;
    TvmCell m_XchgPairCode;     
    T3WDetails m_tip3walletDetails;
    uint32 m_getT3WDetailsCallback;
    bool add = true;
    address m_tip3root;
    TvmCell m_sellPriceCode;
    uint256 m_masterPubKey;
    TvmCell _codeMM;
    TvmCell _dataMM;
    TvmCell _codeMMF;
    TvmCell _dataMMF;
    TvmCell _codeP;
    TvmCell _dataP;
    TvmCell _codeW;
    mapping (uint256=>address) m_addr;
    
    modifier alwaysAccept {
            tvm.accept();
            _;
    }

    function setCodeMM (TvmCell codeMM) external alwaysAccept {
        _codeMM = codeMM;
    }
    
    function setdataMM (TvmCell dataMM) external alwaysAccept {
        _dataMM = dataMM;
    }
    
    function setCodePool (TvmCell codeP) external alwaysAccept {
        _codeP = codeP;
    }
    
    function setDataPool (TvmCell dataP) external alwaysAccept {
        _dataP = dataP;
    }
    
    function setCodeMMF (TvmCell codeMMF) external alwaysAccept {
        _codeMMF = codeMMF;
    }
    
    function setdataMMF (TvmCell dataMMF) external alwaysAccept {
        _dataMMF = dataMMF;
    }
    
    function setCodeW (TvmCell codeW) external alwaysAccept {
        _codeW = codeW;
    }

    function start() public override {
        AddressInput.get(tvm.functionId(setStock), "Enter address of Stock");
    }
    
    function setStock(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        m_stockAddr = value;
        enterPublicKey();
    }

/*    
    function enterPublicKey() public {
        Terminal.input(tvm.functionId(getPublicKey),"Please enter your public key",false);
    }

    function getPublicKey(string value) public {
        uint res;
        bool status;
        (res, status) = stoi("0x"+value);
        if (status && res!=0) {
            m_masterPubKey = res;
            DeployMM();
        } else
            Terminal.input(tvm.functionId(getPublicKey),"Wrong public key. Try again!\nPlease enter your public key",false);
    }
*/

    function enterPublicKey() public {
        UserInfo.getPublicKey(tvm.functionId(getPublicKey));
    }

    function getPublicKey(uint256 value) public {
        Terminal.print(0,format("Your pubkey {:064x}",value));
        m_masterPubKey = value;
        DeployMM();
    }
       
    function DeployMM () public {
        TvmCell _contractMM = tvm.buildStateInit(_codeMM, _dataMM);
        _contractMM = tvm.insertPubkey(_contractMM, m_masterPubKey);
        m_addr[m_masterPubKey] = address.makeAddrStd(0, tvm.hash(_contractMM));
        Sdk.getAccountType(tvm.functionId(MMClientAccType), m_addr[m_masterPubKey]);
    }
    
    function MMClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            ConfirmInput.get(tvm.functionId(isDeployMMClient),"You don't have MM Client. Do you want to deploy it?");
        }else if (acc_type==1){
            Terminal.print(0, "MM Client address is:");
            Terminal.print(tvm.functionId(askTip3), format("{}",m_addr[m_masterPubKey]));
        } else if (acc_type==2){
            Terminal.print(0, format("Account {} is frozen.",m_addr[m_masterPubKey]));
        }
    }
    
    function isDeployMMClient(bool value) public {
        if (value){
            Terminal.print(0, "Send 1 ton or more to the address:");
            Terminal.print(0, format("{}",m_addr[m_masterPubKey]));
            ConfirmInput.get(tvm.functionId(isFCMoneySend),"Did you send the money?");
        }else
            Terminal.print(0, "Terminated! You need MM Client to work with Stock!");
    }
    
    function isFCMoneySend(bool value) public {
        if (value){
            Sdk.getAccountType(tvm.functionId(CheckStatus), m_addr[m_masterPubKey]);
        } else
            Terminal.print(0,"Terminated!");
    }

    function CheckStatus(int8 acc_type) public {
        if (acc_type==-1) {
            isDeployMMClient(true);
        }else if (acc_type==0) {
            Sdk.getBalance(tvm.functionId(checkMMClientBalance), m_addr[m_masterPubKey]);
        }else if (acc_type==1){
            Terminal.print(0, format("Terminated! Account {} is already active.",m_addr[m_masterPubKey]));
        } else if (acc_type==2){
            Terminal.print(0, format("Terminated! Account {} is frozen.",m_addr[m_masterPubKey]));
        }
    }
    
    function checkMMClientBalance(uint128 nanotokens) public {
        if (nanotokens<1 ton) {
            Terminal.print(0, format("Address {} balance is {:t} tons",m_addr[m_masterPubKey],nanotokens));
            isDeployMMClient(true);
        }else {
            Terminal.print(tvm.functionId(DeployMMClientStep2), "Deploying..");
        }
    }
    
    function DeployMMClientStep2() public {
        DeployRootStep2(true);
    }

    function DeployRootStep2(bool value) public {
        if (value)
        {
            TvmCell image = tvm.buildStateInit(_codeMM, _dataMM);
            image = tvm.insertPubkey(image, m_masterPubKey);
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_addr[m_masterPubKey],
                callbackId: tvm.functionId(onSuccessMMDeployed),
                onErrorId: tvm.functionId(onDeployMMFailed),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: m_masterPubKey,
                stateInit: image,
                call: {MarketMaker, m_masterPubKey}
            });
            tvm.sendrawmsg(deployMsg, 1);
        }else
            Terminal.print(0,"Terminated!");
    }
    
    function onSuccessMMDeployed() public {
        Terminal.print(tvm.functionId(askTip3), format("Contract deployed {}.\nNow we need to set config..",m_addr[m_masterPubKey]));
    }

    function onDeployMMFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep2), "Do you want to retry?");
    }
        
    function askTip3() public {
        _MarketMaker = m_addr[m_masterPubKey];
        AddressInput.get(tvm.functionId(setTIP3), "Enter address of tip3root");
    }
    
    function setTIP3(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        m_tip3root = value;
        ConfirmInput.get(tvm.functionId(UpdateInfo), "Do you want to Get Information and send to MM?");
    }
   
    function UpdateInfo(bool value) public {
        if (value == false){
            return;
        }
        ConfirmInput.get(tvm.functionId(getStockTonsCfg),format("Start getStockTonsCfg from {} ?", m_stockAddr));
    }
    
    function getStockTonsCfg(bool value) public {
       if (value == false){
           return;
       }
       Terminal.print(0,format("m_stockAddr = {}",m_stockAddr));
       optional(uint256) none;
       AStock(m_stockAddr).getTonsCfg{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockTonsCfg),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }
    
    function ErrorCatch(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Sad! sdkError {} exitCode {}", sdkError, exitCode));
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
        AddressInput.get(tvm.functionId(getMin), "Enter address of tradingpair");
    }
    
    function getMin(address value) public {
        Terminal.print(0, "Ask for MinAmount!");
        address TrPa = value;
        optional(uint256) none;
        ATradingPair(TrPa).getMinAmount{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setStockMinAmount),
                onErrorId: 0,
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
            onErrorId: 0,
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
            onErrorId: 0,
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
        getXchgPairCode();
    }
    
    function getXchgPairCode() public {
        Terminal.print(0, "Got XchgPairCode!");
        optional(uint256) none;
        AStock(m_stockAddr).getXchgPairCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPairCode),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPairCode(TvmCell value0) public {
        m_XchgPairCode = value0;
        uint256 h = tvm.hash(m_XchgPairCode);
        Terminal.print(tvm.functionId(sendInfo),format("price code hash {:x}",h));
    }
    
    function sendInfo() public {
       Terminal.print(tvm.functionId(sendInfo2), format("Info for MarketMaker! m_stockMinAmount {} m_stockDealsLimit {} MarketMaker {}", m_stockMinAmount, m_stockDealsLimit, _MarketMaker));
    }
    
    function sendInfo2() public view {   
       optional(uint256) none;
       MarketMaker(_MarketMaker).UpdateInfo{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(Update1),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true, 
            pubkey: none} (m_stockTonCfg, m_stockMinAmount, m_stockDealsLimit);
    }
    
    function Update1() public { 
       Terminal.print(tvm.functionId(Update11),format("Continue..."));
    }
    
    function Update11() public view {
       optional(uint256) none;
       MarketMaker(_MarketMaker).UpdateInfo1{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(Update2),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true, 
            pubkey: none} (m_tradingPairCode);
    }
    
    function Update2() public { 
       Terminal.print(tvm.functionId(Update21),format("Continue..."));
    }
    
    function Update21() public view {
       optional(uint256) none;
       MarketMaker(_MarketMaker).UpdateInfo2{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(SetCfg2),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true, 
            pubkey: none} (m_XchgPairCode);
    }
    
    function SetCfg2() public {
        ConfirmInput.get(tvm.functionId(SetCfg5), "Do you want to continue?");
    }

    function SetCfg5(bool value) public {
        if (value == false){ return;}
        ConfirmInput.get(tvm.functionId(getSellPriceCode), "Do you want to getSellPriceCode?");
    }
    
    function getSellPriceCode(bool value) public view {
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
    
    function sendPriceCode(bool value) public view {
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
        ConfirmInput.get(tvm.functionId(AddTip3W), "Do you want to add Tip3wallet?");
    }
    
    function AddTip3W (bool value) public {
        if (value == true){
            AddressInput.get(tvm.functionId(AddTip3), "Enter address of Tip3");
        }
    } 
    
    function AddTip3(address value) public {
        _tip3 = value;
        ConfirmInput.get(tvm.functionId(getT3WDetails), "Confirm?");
    }
    
    function getT3WDetails(bool value) public {
        if (value == false) { return; }
        optional(uint256) none;
        AFlexWallet(_tip3).getDetails{
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
    
    function setT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, 
            uint256 root_public_key, uint256 wallet_public_key, address root_address, 
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, 
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        balance; wallet_public_key; owner_address;lend_ownership;lend_balance;code;allowance;workchain_id;
        m_tip3walletDetails.name = name;
        m_tip3walletDetails.symbol = symbol;
        m_tip3walletDetails.root_public_key = root_public_key;
        m_tip3walletDetails.root_address = root_address;
        m_tip3walletDetails.decimals = decimals;
        Terminal.print(0, format("Got details!"));
        ConfirmInput.get(tvm.functionId(sendTip3Wallet), "Do you want to sendTip3?");
    }
    
    function sendTip3Wallet(bool value) public view {
        if (value == false) {
            return;
        } 
        else {
            optional(uint256) none;
            MarketMaker(_MarketMaker).AddTip3Wallet{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(doNothing),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(m_tip3walletDetails, _tip3);
        }
    }
    
    function doNothing() public{
        Terminal.print(0, format("Send MMClient,Wallet, Pool code and data!"));
        optional(uint256) none;
        
        MarketMaker(_MarketMaker).setCodeflex{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_codeMMF);
            
        MarketMaker(_MarketMaker).setDataflex{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_dataMMF);
            
        MarketMaker(_MarketMaker).setCodepool{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_codeP);
        
        MarketMaker(_MarketMaker).setDatapool{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_dataP);
            
        MarketMaker(_MarketMaker).setCodeW{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_codeW);
        
        ConfirmInput.get(tvm.functionId(deployFlex), "Deploy MMClient for AMM and add wallet code!");
    }
    
    function deployFlex() public view {
         optional(uint256) none;
         MarketMaker(_MarketMaker).DeployFlex{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(preinit),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}();
    }
    
    function preinit() public {
        AddressInput.get(tvm.functionId(init), "Enter address of Notify for init MarketMaker");
    }
    
    function init(address value) public {
        _notify = value;
        uint128 step = 5e8;
        uint128 number = 4;
        optional(uint256) none;
        MarketMaker(_MarketMaker).init{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(presubs),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(m_stockAddr, step, number, _notify);
    }
    
    function presubs() public {
        ConfirmInput.get(tvm.functionId(subs), format("Do you want to subscribe to notification? Notify {} MarketMaker {}", _notify, _MarketMaker));
    }
    
    function subs(bool value) public {
        if (value == false) { finish(); }
        optional(uint256) none;
        AStockNotify(_notify).addSub{
            abiVer: 2,
            extMsg: true, 
            callbackId: tvm.functionId(finish),
            onErrorId: tvm.functionId(ErrorCatch),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none}(_MarketMaker);
    }
    
    function finish() public{    
        Terminal.print(0, format("Success!"));
    }
    
    function getVersion() public override returns (string name, uint24 semver) {
        (name, semver) = ("HelloWorld DeBot", _version(0,1,0));
    }

    function _version(uint24 major, uint24 minor, uint24 fix) private pure inline returns (uint24) {
        return (major << 16) | (minor << 8) | (fix);
    }

    /*
    *  Implementation of Upgradable
    */
    
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}

