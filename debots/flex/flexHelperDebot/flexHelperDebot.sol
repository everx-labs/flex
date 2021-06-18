pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "../interfaces/Debot.sol";
import "../interfaces/Upgradable.sol";
import "../interfaces/Transferable.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/ConfirmInput.sol";
import "../interfaces/AddressInput.sol";
import "../interfaces/AmountInput.sol";

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}


abstract contract AMSig {
    function submitTransaction(
        address dest,
        uint128 value,
        bool bounce,
        bool allBalance,
        TvmCell payload)
    public returns (uint64 transId) {}
}

abstract contract AFlex {
    function getTradingPairCode() public returns(TvmCell value0) {}
    function getXchgPairCode() public returns(TvmCell value0) {}    
    function getNotifyAddr() public returns(address value0) {}
    function getMinAmount() public returns(uint128 value0) {}
    function getDealsLimit() public returns(uint8 value0) {}
    function getTonsCfg() public returns(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
}

abstract contract ATip3Root {
    function getSymbol() public returns(bytes value0){}
    function getWalletAddress(int8 workchain_id, uint256 pubkey, uint256 owner_std_addr) public returns(address value0) {}
    function deployEmptyWallet(uint32 _answer_id, int8 workchain_id, uint256 pubkey, uint256 internal_owner, uint128 grams) public functionID(1745532193) returns(address value0) {}
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code, TvmCell xchg_pair_code) public {}
    function deployTradingPair(address tip3_root, uint128 deploy_min_value, uint128 deploy_value) public returns(address value0) {}
    function deployXchgPair(address tip3_major_root, address tip3_minor_root, uint128 deploy_min_value, uint128 deploy_value) public returns(address value0) {}
    function setFLeXCfg(StockTonCfg tons_cfg,address stock, address notify_addr) public {}
    function getFLeX() public returns(address value0) {}
    function transfer(address dest, uint128 value, bool bounce) public {}
}


interface IFlexDebot {
    function onGetFCAddressAndKeys(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, TvmCell tpcode, TvmCell xchgpcode) external;
    function onGetTip3WalletAddress(address t3w) external;
    function updateTradingPairs() external;
}

struct LendOwnership{
   address owner;
   uint128 lend_balance;
   uint32 lend_finish_time;
}
struct Allowance{
    address spender;
    uint128 remainingTokens;
}

struct T3WDetails {
    string name;
    string symbol;
    uint8 decimals;
    uint128 balance;
    LendOwnership lend_ownership;
    uint256 rootPubKey;
    address rootAddress;
    TvmCell code;
}

abstract contract ATip3Wallet {
    function getDetails() public returns (bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) {}
}

contract FlexHelperDebot is Debot, Upgradable, Transferable {

    uint256 constant TIP3ROOT_CODEHASH = 0x9e4be18bf6a7f77d5267dde7afa144f905f46a7aa82854df8846ea21f71701c9;

    address m_stockAddr;
    string m_t3Symbol;
    uint256 m_masterPubKey;
    address m_sender;
    TvmCell m_flexClientCode;
    uint128 m_tradingPairDeploy;

    TvmCell m_sendMsg;
    address m_flexClient;
    address m_tip3root;
    address m_majorTip3root;
    address m_minorTip3root;    
    TvmCell m_tradingPairCode;
    TvmCell m_xchgPairCode;
    address m_tip3wallet;
    address m_msig;
    uint128 m_dealTons;
    StockTonCfg m_stockTonCfg;
    address m_stockNotifyAddr;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    mapping(uint8 => T3WDetails) m_tip3walletDetails;//mapping for reduce gas in constructor

    function setStockAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_stockAddr = addr;
    }

    function setFlexClientCode(TvmCell code) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexClientCode = code;
    }

    function start() public override {
        Terminal.print(0,"Sorry, I can't help you. Bye!");
    }

    function getFCAddressAndKeys(address stock) public {
        m_stockAddr = stock;
        m_sender = msg.sender;
        enterPublicKey();        
    }

    function enterPublicKey() public {
        Terminal.input(tvm.functionId(getPublicKey),"Please enter your public key",false);
    }

    function getPublicKey(string value) public {
        uint res;
        bool status;
        (res, status) = stoi("0x"+value);
        if (status && res!=0) {
            m_masterPubKey = res;
            getTradingPairCode();
        } else
            Terminal.input(tvm.functionId(getPublicKey),"Wrong public key. Try again!\nPlease enter your public key",false);
    }

    function getTradingPairCode() public {
        optional(uint256) none;
        AFlex(m_stockAddr).getTradingPairCode{
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
        optional(uint256) none;
        AFlex(m_stockAddr).getXchgPairCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPairCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPairCode(TvmCell value0) public {
            m_xchgPairCode = value0;    
            getStockTonsCfg();
    }

    function getStockTonsCfg() view public {
        optional(uint256) none;
        AFlex(m_stockAddr).getTonsCfg{
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

    function setStockTonsCfg(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) public {
        m_stockTonCfg.transfer_tip3 = transfer_tip3;
        m_stockTonCfg.return_ownership = return_ownership;
        m_stockTonCfg.trading_pair_deploy = trading_pair_deploy;
        m_stockTonCfg.order_answer = order_answer;
        m_stockTonCfg.process_queue = process_queue;
        m_stockTonCfg.send_notify = send_notify;
        
        optional(uint256) none;
        AFlex(m_stockAddr).getMinAmount{
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
        m_stockMinAmount = value0;

        optional(uint256) none;
        AFlex(m_stockAddr).getDealsLimit{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getStockNotifyAddr),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getStockNotifyAddr(uint8 value0) public {
        m_stockDealsLimit = value0;

        optional(uint256) none;
        AFlex(m_stockAddr).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockNotifyAddr),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setStockNotifyAddr(address value0) public {
        m_stockNotifyAddr=value0;
                
        TvmCell deployState = tvm.insertPubkey(m_flexClientCode, m_masterPubKey);
        m_flexClient = address.makeAddrStd(0, tvm.hash(deployState));
        Sdk.getAccountType(tvm.functionId(FlexClientAccType), m_flexClient);
    }

    function FlexClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            ConfirmInput.get(tvm.functionId(isDeployFlexClient),"You don't have FLeX Client. Do you want to deploy it?");
        }else if (acc_type==1){
            Terminal.print(0, "FLeX Client address is:");
            Terminal.print(tvm.functionId(checkFlexClientStockCfg), format("{}",m_flexClient));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(enterPublicKey), format("Account {} is frozen.",m_flexClient));
        }
    }

    function isDeployFlexClient(bool value) public {
        if (value){
            Terminal.print(0, "Send 1 ton or more to the address:");
            Terminal.print(0, format("{}",m_flexClient));
            ConfirmInput.get(tvm.functionId(isFCMoneySend),"Did you send the money?");
        }else
            Terminal.print(tvm.functionId(enterPublicKey), "Terminated! You need FLeX Client to work with Stock!");
    }

    function isFCMoneySend(bool value) public {
        if (value){
            Sdk.getAccountType(tvm.functionId(DeployFlexClientStep1), m_flexClient);
        } else
            Terminal.print(tvm.functionId(enterPublicKey),"Terminated!");
    }

    function DeployFlexClientStep1(int8 acc_type) public {
        if (acc_type==-1) {
            isDeployFlexClient(true);
        }else if (acc_type==0) {
            Sdk.getBalance(tvm.functionId(checkFlexClientBalance), m_flexClient);
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(enterPublicKey), format("Terminated! Account {} is already active.",m_flexClient));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(enterPublicKey), format("Terminated! Account {} is frozen.",m_flexClient));
        }
    }

    function checkFlexClientBalance(uint128 nanotokens) public {
        if (nanotokens<1 ton) {
            Terminal.print(0, format("Address {} balance is {:t} tons",m_flexClient,nanotokens));
            isDeployFlexClient(true);
        }else {
            Terminal.print(tvm.functionId(DeployFlexClientStep2Proxy), "Deploying..");
        }
    }


    function DeployFlexClientStep2Proxy() public {
        DeployRootStep2(true);
    }

    function DeployRootStep2(bool value) public {
        if (value)
        {
            TvmCell image = tvm.insertPubkey(m_flexClientCode, m_masterPubKey);
            optional(uint256) none;
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_flexClient,
                callbackId: tvm.functionId(onSuccessFCDeployed),
                onErrorId: tvm.functionId(onDeployFCFailed),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                stateInit: image,
                call: {AFlexClient,m_masterPubKey,m_tradingPairCode,m_xchgPairCode}
            });
            tvm.sendrawmsg(deployMsg, 1);
        }else
            Terminal.print(0,"Terminated!");
    }

    function onSuccessFCDeployed() public {
        Terminal.print(tvm.functionId(flexClientSetStockCfg), format("Contract deployed {}.\nNow we need to set config..",m_flexClient));
    }

    function onDeployFCFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep2), "Do you want to retry?");
    }

    function flexClientSetStockCfg() public {
        optional(uint256) none;
        TvmCell sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onSuccessFCSetTonsCfg),
            onErrorId: tvm.functionId(onFCSetTonsCfgError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {AFlexClient.setFLeXCfg,m_stockTonCfg,m_stockAddr,m_stockNotifyAddr}
        });
        tvm.sendrawmsg(sendMsg, 1);
    }

    function onSuccessFCSetTonsCfg() public {
        Terminal.print(tvm.functionId(onGetFCAddressAndKeys), "Success!");
    }

    function onFCSetTonsCfgError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setTonsCfg failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(flexClientSetStockCfg), "Retrying..");
    }

    function checkFlexClientStockCfg() public {
        optional(uint256) none;
        AFlexClient(m_flexClient).getFLeX{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getFlexFlex),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getFlexFlex(address value0) public {
        if (value0!=m_stockAddr){
            Terminal.print(tvm.functionId(flexClientSetStockCfg), "We need to set config for you flex client..");
        }else{
            onGetFCAddressAndKeys();
        }
    }

    function onGetFCAddressAndKeys() public {
        IFlexDebot(m_sender).onGetFCAddressAndKeys(m_flexClient,m_masterPubKey,m_stockMinAmount,m_stockDealsLimit,m_stockTonCfg,m_tradingPairCode,m_xchgPairCode);
        m_sender = address(0);
    }

//get tip3 wallet address
    function getTip3WalletAddress(address stock, address tip3root, address flexClient, StockTonCfg tonCfg) public {
        m_stockAddr = stock;
        m_tip3root = tip3root;
        m_flexClient = flexClient;
        m_stockTonCfg = tonCfg;
        m_sender = msg.sender;
        
        optional(uint256) none;
        ATip3Root(m_tip3root).getSymbol{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3RootSymbol),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();        
    }

    function setTip3RootSymbol(bytes value0) public {
        m_t3Symbol = value0;
        optional(uint256) none;
        ATip3Root(m_tip3root).getWalletAddress{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3WalletAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(0,0x0,m_flexClient.value);
    }

    function setTip3WalletAddress(address value0) public {
        m_tip3wallet = value0;
        Sdk.getAccountType(tvm.functionId(Tip3WalletAccType),m_tip3wallet);
    }

    function Tip3WalletAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            ConfirmInput.get(tvm.functionId(isDeployTip3Wallet),format("You don't have {} tip3 wallet. Do you want to deploy it?",m_t3Symbol));
        }else if (acc_type==1){
            onGetTip3WalletAddress();
        } else if (acc_type==2){
            Terminal.print(0, format("{} tip3 wallet {} is frozen.",m_t3Symbol,m_flexClient));
        }
    }

    function isDeployTip3Wallet(bool value) public {
        if (value){
            AddressInput.get(tvm.functionId(enterMsigAddr),"Enter your msig address to pay for deployment");
        }else
            Terminal.print(tvm.functionId(Debot.start), "Terminated!");
    }

    function enterMsigAddr(address value) public {
        m_msig = value;
        AmountInput.get(tvm.functionId(enterMsigTons), "How many tons your would like to transfer as maintenance?\n1 additional ton will transafered for execution fee, not used part will be returned",9,1000000000,100000000000);
    }

    function enterMsigTons(uint128 value) public {
        m_dealTons = value;
        deployEmptyWallet();
    }

    function deployEmptyWallet() view public {
        TvmCell payload = tvm.encodeBody(ATip3Root.deployEmptyWallet, 0, 0, 0x0, m_flexClient.value, m_dealTons);
        optional(uint256) none;
        uint128 amount = m_dealTons+1000000000;
        AMSig(m_msig).submitTransaction{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onDEWSuccess),
                onErrorId: tvm.functionId(onDEWError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none
            }(m_tip3root, amount, true, false, payload);
    }

    function onDEWSuccess (uint64 transId)  public  {
        Terminal.print(0,format("Tip3 {} wallet address:",m_t3Symbol));
        Terminal.print(0,format("{}",m_tip3wallet));
        ConfirmInput.get(tvm.functionId(checkT3WWait),"Please wait a little. Continue?");
    }

    function onDEWError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Empty wallet deploy failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        Terminal.print(tvm.functionId(deployEmptyWallet), "Retrying..");
    }

    function checkT3WWait(bool value) public {
        if (value)
            Sdk.getAccountType(tvm.functionId(Tip3WalletAccTypeWait),m_tip3wallet);
        else Terminal.print(0, "Terminated!");
    }

    function Tip3WalletAccTypeWait(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            ConfirmInput.get(tvm.functionId(checkT3WWait),"Please wait a little. Continue?");
        }else if (acc_type==1){
            onGetTip3WalletAddress();
        } else {
            Terminal.print(0, "Unexpected error.");
        }
    }

    function onGetTip3WalletAddress() public {
        IFlexDebot(m_sender).onGetTip3WalletAddress(m_tip3wallet);
        m_sender = address(0);
    }

/// deploy trading pair
    function deployTradigPair(address stock, address fclient, uint128 tpd, uint256 pk) public
    {
        m_masterPubKey = pk;
        m_tradingPairDeploy = tpd;
        m_stockAddr = stock;
        m_flexClient = fclient;
        m_sender = msg.sender;
        Sdk.getBalance(tvm.functionId(checkFlexClientBalanceForTPDeploy), m_flexClient);
    }

    function checkFlexClientBalanceForTPDeploy(uint128 nanotokens) public {
        if (nanotokens<2.3 ton) {
            Terminal.print(tvm.functionId(onDeployTradigPair), format("Error: your flex client balance is {:t}\nFor trading pair deploy it should be more than 2.3 ton",nanotokens));
        }else {
            AddressInput.get(tvm.functionId(enterTip3RootAddress),"Please enter your tip3 root address");
        }
    }

    function enterTip3RootAddress(address value) public {
        m_tip3root = value;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3RootCodeHash), m_tip3root);
    }

    function getTip3RootCodeHash(uint256 code_hash) public {        
        if (code_hash == TIP3ROOT_CODEHASH){
            uint128 payout = m_tradingPairDeploy+1000000000;
            optional(uint256) none;
            m_sendMsg =  tvm.buildExtMsg({
                abiVer: 2,
                dest: m_flexClient,
                callbackId: tvm.functionId(onDeployTPSuccess),
                onErrorId: tvm.functionId(onDeployTPError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                call: {AFlexClient.deployTradingPair, m_tip3root, m_tradingPairDeploy, payout}
            });
            ConfirmInput.get(tvm.functionId(confirmDeployTradingPair),format("Deploy will cost {:t}. Continue?",payout));
        }else{
            Terminal.print(tvm.functionId(onDeployTradigPair), "Wrong Tip3 Root contract. Please, use Tip3 Debot (0:c3ce4ec1dde824b580985795c805e2f41f30aa7ff70207f3a99ca184871f7d86) to depoly Tip3 Root");
        }       
    }

    function confirmDeployTradingPair(bool value) public {
        if (value) {
            onSendDeployTradingPair();
        }else {
            onDeployTradigPair();
        }
    }

    function onSendDeployTradingPair() public {
        tvm.sendrawmsg(m_sendMsg, 1);
    }

    function onDeployTPSuccess (address value0)  public  {
        Terminal.print(0,"Deploy trading pair success!");
        onDeployTradigPair();
    }
    function onDeployTPError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy trading pair failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployTradingPair),"Do you want ot retry?");
    }

    function onDeployTradigPair() public {
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }

/// deploy xchg trading pair
    function deployXchgTradigPair(address stock, address fclient, uint128 tpd, uint256 pk) public
    {
        m_masterPubKey = pk;
        m_tradingPairDeploy = tpd;
        m_stockAddr = stock;
        m_flexClient = fclient;
        m_sender = msg.sender;
        Sdk.getBalance(tvm.functionId(checkFlexClientBalanceForXchgTPDeploy), m_flexClient);
    }

    function checkFlexClientBalanceForXchgTPDeploy(uint128 nanotokens) public {
        if (nanotokens<2.3 ton) {
            Terminal.print(tvm.functionId(onDeployXchgTradigPair), format("Error: your flex client balance is {:t}\nFor trading pair deploy it should be more than 2.3 ton",nanotokens));
        }else {
            AddressInput.get(tvm.functionId(enterMajorTip3RootAddress),"Please enter major tip3 root address");
        }
    }

    function enterMajorTip3RootAddress(address value) public {
        m_majorTip3root = value;
        Sdk.getAccountCodeHash(tvm.functionId(getMajorTip3RootCodeHash), m_majorTip3root);
    }

    function getMajorTip3RootCodeHash(uint256 code_hash) public {     
        if (code_hash == TIP3ROOT_CODEHASH){
            AddressInput.get(tvm.functionId(enterMinorTip3RootAddress),"Please enter minor tip3 root address");
        }else{
            Terminal.print(tvm.functionId(onDeployXchgTradigPair), "Wrong Tip3 Root contract. Please, use Tip3 Debot to depoly Tip3 Root");
        }       
    }

    function enterMinorTip3RootAddress(address value) public {
        m_minorTip3root = value;
        Sdk.getAccountCodeHash(tvm.functionId(getMinorTip3RootCodeHash), m_minorTip3root);
    }

    function getMinorTip3RootCodeHash(uint256 code_hash) public { 
        if (code_hash == TIP3ROOT_CODEHASH){
            uint128 payout = m_tradingPairDeploy+1000000000;
            optional(uint256) none;
            m_sendMsg =  tvm.buildExtMsg({
                abiVer: 2,
                dest: m_flexClient,
                callbackId: tvm.functionId(onDeployXchgTPSuccess),
                onErrorId: tvm.functionId(onDeployXchgTPError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                call: {AFlexClient.deployXchgPair, m_majorTip3root, m_minorTip3root, m_tradingPairDeploy, payout}
            });
            ConfirmInput.get(tvm.functionId(confirmDeployXchgTradingPair),format("Deploy will cost {:t}. Continue?",payout));
        }else{
            Terminal.print(tvm.functionId(onDeployXchgTradigPair), "Wrong Tip3 Root contract. Please, use Tip3 Debot to depoly Tip3 Root");
        }       
    }



    function confirmDeployXchgTradingPair(bool value) public {
        if (value) {
            onSendDeployXchgTradingPair();
        }else {
            onDeployXchgTradigPair();
        }
    }

    function onSendDeployXchgTradingPair() public {
        tvm.sendrawmsg(m_sendMsg, 1);
    }

    function onDeployXchgTPSuccess (address value0)  public  {
        Terminal.print(0,"Deploy trading pair success!");
        onDeployXchgTradigPair();
    }
    function onDeployXchgTPError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy trading pair failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployXchgTradingPair),"Do you want ot retry?");
    }

    function onDeployXchgTradigPair() public {
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }

//withdraw tons
    function withdrawTons(address fclient) public {
        m_flexClient = fclient;
        m_sender = msg.sender;
        AddressInput.get(tvm.functionId(enterWithdrawalAddr),"Please enter your withdrawal address");
    }


    function enterWithdrawalAddr(address value) public {
        m_msig = value;
        AmountInput.get(tvm.functionId(enterWithdrawalTons),"Please enter ammount of tons you want to withdraw",9,10000000,0xFFFFFFFFFFFFFFFF);
    }

    function enterWithdrawalTons(uint128 value) public {
        m_dealTons = value;
        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
                abiVer: 2,
                dest: m_flexClient,
                callbackId: tvm.functionId(onWithdrawSuccess),
                onErrorId: tvm.functionId(onWithdrawError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                call: {AFlexClient.transfer, m_msig, m_dealTons, true}
            });

        ConfirmInput.get(tvm.functionId(confirmWithdrawTons),format("Do you want to transfer {:t} tons from your flex client to address {}?",m_dealTons,m_msig));
    }

    function confirmWithdrawTons(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        }else {
            onWithdrawTons();
        }
    }
    function onWithdrawSuccess ()  public  {
        Terminal.print(tvm.functionId(onWithdrawTons),"Success!");
    }
    function onWithdrawError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Transfer failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmWithdrawTons),"Do you want ot retry?");
    }
    function onWithdrawTons() public {
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Helper Debot";
        version = "0.1.7";
        publisher = "TON Labs";
        key = "Helps Flex to calculate addresses and collect data";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, i'am a Flex Helper DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = "";
    }

    function getRequiredInterfaces() public view override returns (uint256[] interfaces) {
        return [ AddressInput.ID, AmountInput.ID, ConfirmInput.ID, Sdk.ID, Terminal.ID ];
    }
    /*
    *  Implementation of Upgradable
    */
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}
