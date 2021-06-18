pragma ton-solidity >=0.35.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "../interfaces/Debot.sol";
import "../interfaces/Upgradable.sol";
import "../interfaces/Transferable.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/Menu.sol";
import "../interfaces/ConfirmInput.sol";
import "../interfaces/Msg.sol";
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

abstract contract AStock {
    function getTradingPairCode() public returns(TvmCell value0) {}
    function getNotifyAddr() public returns(address value0) {}
    function getTonsCfg() public returns(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
}

abstract contract ATip3Root {
    function getWalletAddress(int8 workchain_id, uint256 pubkey, uint256 owner_std_addr) public returns(address value0) {}
    function deployEmptyWallet(uint32 _answer_id, int8 workchain_id, uint256 pubkey, uint256 internal_owner, uint128 grams) public functionID(1745532193) returns(address value0) {}
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code) public {}
    function deployTradingPair(address stock_addr, address tip3_root, uint128 deploy_min_value, uint128 deploy_value) public returns(address value0) {}
    function setStockCfg(StockTonCfg tons_cfg,address stock, address notify_addr) public {}
}

interface IFlexDebot {
    function onGetFCAddressAndKeys(address fc, uint256 pk, uint256 sk) external;
    function onGetTip3WalletAddress(address t3w) external;
    function getTradingPairCodeHash() external;
}

contract FlexHelperDebot is Debot, Upgradable, Transferable {

    address m_stockAddr;
    string m_seedphrase;
    uint256 m_masterPubKey;
    uint256 m_masterSecKey;
    address m_sender;
    TvmCell m_flexClientCode;
    uint128 m_tradingPairDeploy;
    TvmCell m_sendMsg;
    address m_flexClient;
    address m_tip3root;
    TvmCell m_tradingPairCode;
    address m_tip3wallet;
    address m_msig;
    uint128 m_dealTons;
    StockTonCfg m_stockTonCfg;
    address m_stockNotifyAddr;

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
        Terminal.print(0,"Sorry, I can't help  you. Bye!");
    }

    function getFCAddressAndKeys(address stock) public {
        m_stockAddr = stock;
        m_sender = msg.sender;
        Menu.select("","At this step you need to have a seed phrase that will be used to work with FLeX. Keep it in secret!",[
            MenuItem("Generate a seed phrase for me","",tvm.functionId(menuGenSeedPhrase)),
            MenuItem("I have the seed phrase","",tvm.functionId(menuEnterSeedPhrase))
        ]);
    }

    function menuGenSeedPhrase(uint32 index) public {
        Sdk.mnemonicFromRandom(tvm.functionId(showMnemonic),1,12);
    }

    function showMnemonic(string phrase) public {
        string str = "Generated seed phrase > ";
        str.append(phrase);
        str.append("\nWarning! Please don't forget it!");
        Terminal.print(0,str);
        menuEnterSeedPhrase(0);
    }

    function menuEnterSeedPhrase(uint32 index) public {
        Terminal.input(tvm.functionId(checkSeedPhrase),"Enter your seed phrase",false);
    }

    function checkSeedPhrase(string value) public {
        m_seedphrase = value;
        Sdk.mnemonicVerify(tvm.functionId(verifySeedPhrase),m_seedphrase);
    }

    function verifySeedPhrase(bool valid) public {
        if (valid){
            getMasterKeysFromMnemonic(m_seedphrase);
        }else{
            Terminal.print(tvm.functionId(Debot.start),"Error: not valid seed phrase! (try to enter it without quotes or generate a new one)");
        }
    }

    function getMasterKeysFromMnemonic(string phrase) public {
        Sdk.hdkeyXprvFromMnemonic(tvm.functionId(getMasterKeysFromMnemonicStep1),phrase);
    }

    function getMasterKeysFromMnemonicStep1(string xprv) public {
        string path = "m/44'/396'/0'/0/0";
        Sdk.hdkeyDeriveFromXprvPath(tvm.functionId(getMasterKeysFromMnemonicStep2), xprv, path);
    }

    function getMasterKeysFromMnemonicStep2(string xprv) public {
        Sdk.hdkeySecretFromXprv(tvm.functionId(getMasterKeysFromMnemonicStep3), xprv);
    }

    function getMasterKeysFromMnemonicStep3(uint256 sec) public {
        Sdk.naclSignKeypairFromSecretKey(tvm.functionId(getMasterKeysFromMnemonicStep4), sec);
    }

    function getMasterKeysFromMnemonicStep4(uint256 sec, uint256 pub) public {
        m_masterPubKey = pub;
        m_masterSecKey = sec;
        checkFlexClient();
    }

    function checkFlexClient() public {
        TvmCell deployState = tvm.insertPubkey(m_flexClientCode, m_masterPubKey);
        m_flexClient = address.makeAddrStd(0, tvm.hash(deployState));
        Sdk.getAccountType(tvm.functionId(FlexClientAccType), m_flexClient);
    }

    function FlexClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            getStockNotifyAddr();
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(onGetFCAddressAndKeys), format("FLeX Client account is {}",m_flexClient));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(Debot.start), format("todo Account {} is frozen.",m_flexClient));
        }
    }

    function getStockNotifyAddr() view public {
        optional(uint256) none;
        AStock(m_stockAddr).getNotifyAddr{
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
        getTradingPairCode();
    }

    function getTradingPairCode() public {
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
        getStockTonsCfg();
    }

    function getStockTonsCfg() view public {
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

    function setStockTonsCfg(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) public {
        m_stockTonCfg.transfer_tip3 = transfer_tip3;
        m_stockTonCfg.return_ownership = return_ownership;
        m_stockTonCfg.trading_pair_deploy = trading_pair_deploy;
        m_stockTonCfg.order_answer = order_answer;
        m_stockTonCfg.process_queue = process_queue;
        m_stockTonCfg.send_notify = send_notify;
        ConfirmInput.get(tvm.functionId(isDeployFlexClient),"You have no FLeX Client. Do you want to deploy it?");
    }

    function isDeployFlexClient(bool value) public {
        if (value){
            ConfirmInput.get(tvm.functionId(isFCMoneySend),format("Send some tokens to address {}\nDid you send the money?",m_flexClient));
        }else
            Terminal.print(tvm.functionId(Debot.start), "Terminated! You need FLeX Client to work with Stock!");
    }

    function isFCMoneySend(bool value) public {
        if (value){
            Sdk.getAccountType(tvm.functionId(DeployFlexClientStep1), m_flexClient);
        } else
            Terminal.print(0,"Terminated!");
    }

    function DeployFlexClientStep1(int8 acc_type) public {
        if (acc_type==-1) {
            isDeployFlexClient(true);
        }else if (acc_type==0) {
            Terminal.print(tvm.functionId(DeployFlexClientStep2Proxy), "Deploying..");
        }else if (acc_type==1){
            Terminal.print(0, format("Terminated! Account {} is already active.",m_flexClient));
        } else if (acc_type==2){
            Terminal.print(0, format("Terminated! Account {} is frozen.",m_flexClient));
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

                call: {AFlexClient,m_masterPubKey,m_tradingPairCode}
            });
            Msg.sendWithKeypair(tvm.functionId(onSuccessFCDeployed), deployMsg,m_masterPubKey,m_masterSecKey);
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
            call: {AFlexClient.setStockCfg,m_stockTonCfg,m_stockAddr,m_stockNotifyAddr}
        });
        Msg.sendWithKeypair(tvm.functionId(onSuccessFCDeployed),sendMsg,m_masterPubKey,m_masterSecKey);
    }

    function onSuccessFCSetTonsCfg() public {
        Terminal.print(tvm.functionId(onGetFCAddressAndKeys), "Success!");
    }

    function onFCSetTonsCfgError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setTonsCfg failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(flexClientSetStockCfg), "Retrying..");
    }

    function onGetFCAddressAndKeys() public {
        IFlexDebot(m_sender).onGetFCAddressAndKeys(m_flexClient,m_masterPubKey,m_masterSecKey);
        m_sender = address(0);
    }

//get tip3 wallet address
    function getTip3WalletAddress(address tip3root, address flexClient) public {
        m_tip3root = tip3root;
        m_flexClient = flexClient;
        m_sender = msg.sender;

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
            ConfirmInput.get(tvm.functionId(isDeployTip3Wallet),"You have no tip3 wallet. Do you want to deploy it?");
        }else if (acc_type==1){
            onGetTip3WalletAddress();
        } else if (acc_type==2){
            Terminal.print(0, format("tip3 wallet {} is frozen.",m_flexClient));
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
        AmountInput.get(tvm.functionId(enterMsigTons), "How many tons your would like to transfer as maintenance?\nOne additional ton will be transaverd for execution fee, not used part will be returned",9,1000000000,100000000000);
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
        ConfirmInput.get(tvm.functionId(checkT3WWait),format("tip 3 wallet address {}\nPlease wait a little.Continue?",m_tip3wallet));
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
            ConfirmInput.get(tvm.functionId(checkT3WWait),"Please wait a little.Continue?");
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
    function deployTradigPair(address stock, address fclient, uint128 tpd, uint256 pk, uint256 sk) public
    {
        m_masterPubKey = pk;
        m_masterSecKey = sk;
        m_tradingPairDeploy = tpd;
        m_stockAddr = stock;
        m_flexClient = fclient;
        m_sender = msg.sender;
        AddressInput.get(tvm.functionId(enterTip3RootAddress),"Please enter your tip3 root address");
    }

    function enterTip3RootAddress(address value) public {
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
            call: {AFlexClient.deployTradingPair, m_stockAddr, value, m_tradingPairDeploy, payout}
        });

        ConfirmInput.get(tvm.functionId(confirmDeployTradingPair),format("Deploy will be cost {:t}. Continue?",payout));
    }

    function confirmDeployTradingPair(bool value) public {
        if (value) {
            onSendDeployTradingPair();
        }else {
            onDeployTradigPair();
        }
    }

    function onSendDeployTradingPair() public {
        Msg.sendWithKeypair(tvm.functionId(onDeployTPSuccess),m_sendMsg,m_masterPubKey,m_masterSecKey);
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
        IFlexDebot(m_sender).getTradingPairCodeHash();
        m_sender = address(0);
    }

    /*
    *  Implementation of DeBot
    */

    function getVersion() public override returns (string name, uint24 semver) {
        (name, semver) = ("Flex Helper DeBot", _version(0,0,5));
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
