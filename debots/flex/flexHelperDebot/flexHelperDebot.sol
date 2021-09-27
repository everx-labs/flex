pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../abstract/Transferable.sol";
import "../abstract/AMSig.sol";
import "../abstract/AFlex.sol";
import "../abstract/AFlexClient.sol";
import "../abstract/AWrapper.sol";
import "../interface/IFlexDebot.sol";
import "../interface/IFlexHelperDebot.sol";

contract FlexHelperDebot is Debot, Upgradable, Transferable, IFlexHelperDebot {

    uint8 constant ADDR_FLEX = 0;
    uint8 constant ADDR_SENDER = 1;
    uint8 constant ADDR_FLEX_CLIENT = 2;
    uint8 constant ADDR_TIP3_ROOT = 3;
    uint8 constant ADDR_TIP3_MAJOR_ROOT = 4;
    uint8 constant ADDR_TIP3_MINOR_ROOT = 5;
    uint8 constant ADDR_TIP3_WALLET = 6;
    uint8 constant ADDR_MSIG = 7;
    mapping (uint8=>address) m_addr;

    string m_t3Symbol;
    uint256 m_masterPubKey;
    uint32 m_signingBox;
    TvmCell m_flexClientCode;
    uint128 m_tradingPairDeploy;
    TvmCell m_sendMsg;
    TvmCell m_tradingPairCode;
    TvmCell m_xchgPairCode;
    TvmCell m_flexWalletCode;
    uint128 m_dealTons;
    FlexTonCfg m_flexTonCfg;
    uint128 m_minTradeAmount;
    uint8 m_flexDealsLimit;

    function setFlexClientCode(TvmCell code) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexClientCode = code;
    }

    function setFlexWalletCode(TvmCell code) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexWalletCode = code;
    }

    function start() public override {
        Terminal.print(0,"Sorry, I can't help you. Bye!");
    }

    function getFCAddressAndKeys(address flex) public override {
        m_addr[ADDR_FLEX] = flex;
        m_addr[ADDR_SENDER] = msg.sender;
        UserInfo.getPublicKey(tvm.functionId(getPublicKey));
    }

    function getPublicKey(uint256 value) public {
        m_masterPubKey = value;
        UserInfo.getSigningBox(tvm.functionId(getSigningBox));
    }

    function getSigningBox(uint32 handle) public {
        m_signingBox = handle;
        getTradingPairCode();
    }

    function getTradingPairCode() public view {
        optional(uint256) none;
        AFlex(m_addr[ADDR_FLEX]).getTradingPairCode{
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
        AFlex(m_addr[ADDR_FLEX]).getXchgPairCode{
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
            getFlexTonsCfg();
    }

    function getFlexTonsCfg() view public {
        optional(uint256) none;
        AFlex(m_addr[ADDR_FLEX]).getTonsCfg{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setFlexTonsCfg),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setFlexTonsCfg(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) public {
        m_flexTonCfg.transfer_tip3 = transfer_tip3;
        m_flexTonCfg.return_ownership = return_ownership;
        m_flexTonCfg.trading_pair_deploy = trading_pair_deploy;
        m_flexTonCfg.order_answer = order_answer;
        m_flexTonCfg.process_queue = process_queue;
        m_flexTonCfg.send_notify = send_notify;

        optional(uint256) none;
        AFlex(m_addr[ADDR_FLEX]).getDealsLimit{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getFlexDealsLimit),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getFlexDealsLimit(uint8 value0) public {
        m_flexDealsLimit = value0;

        TvmCell deployState = tvm.insertPubkey(m_flexClientCode, m_masterPubKey);
        m_addr[ADDR_FLEX_CLIENT] = address.makeAddrStd(0, tvm.hash(deployState));
        Sdk.getAccountType(tvm.functionId(FlexClientAccType), m_addr[ADDR_FLEX_CLIENT]);
    }

    function FlexClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            Terminal.print(0,"You don't have Flex Client. Let's deploy it!");
            UserInfo.getAccount(tvm.functionId(deployFlexClient));
        }else if (acc_type==1){
            Terminal.print(0, "Flex Client address is:");
            Terminal.print(tvm.functionId(checkFlexClientStockCfg), format("{}",m_addr[ADDR_FLEX_CLIENT]));
        } else if (acc_type==2){
            Terminal.print(0, format("Account {} is frozen.",m_addr[ADDR_FLEX_CLIENT]));
        }
    }

    function deployFlexClient(address value) public {
        m_addr[ADDR_MSIG] = value;
        Terminal.print(0,format("TONs will be transfered from your multisig wallet {}",m_addr[ADDR_MSIG]));
        Sdk.getAccountType(tvm.functionId(clientAccType), m_addr[ADDR_MSIG]);
    }

    function clientAccType(int8 acc_type) public {
        if (acc_type==1) {
            Sdk.getBalance(tvm.functionId(getClientBalance), m_addr[ADDR_MSIG]);
        }else {
            Terminal.print(0, "Error: Address attached to your account not active!");
        }
    }

    function getClientBalance(uint128 nanotokens) public {
        if (nanotokens>2 ton) {
            AmountInput.get(tvm.functionId(getBalanceToSend), "How many tons you want to send to FlexClient for trade operations:",9,1 ton,nanotokens-1 ton);
        } else {
            Terminal.print(0, "Error: Your msig balance should be more than 2 TONs!");
        }
    }

    function getBalanceToSend(uint128 value) public {
        m_dealTons = value;
        grantFlexClient();
    }

    function grantFlexClient() view public {
        optional(uint256) none;
        TvmCell payload;
        AMSig(m_addr[ADDR_MSIG]).submitTransaction{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onFCGrantSuccess),
                onErrorId: tvm.functionId(onFCGrantError),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none
            }(m_addr[ADDR_FLEX_CLIENT], m_dealTons, false, false, payload);
    }

    function onFCGrantSuccess (uint64 transId)  public  {
        transId;//disable compile warning
        Terminal.print(0,"Deploying flex client");
        this.deployFlexClientCode(true);
    }

    function onFCGrantError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Transaction failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        Terminal.print(tvm.functionId(grantFlexClient), "Retrying..");
    }

    function deployFlexClientCode(bool value) public {
        if (value)
        {
            TvmCell image = tvm.insertPubkey(m_flexClientCode, m_masterPubKey);
            optional(uint256) none;
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_addr[ADDR_FLEX_CLIENT],
                callbackId: tvm.functionId(onSuccessFCDeployed),
                onErrorId: tvm.functionId(onDeployFCFailed),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none,
                stateInit: image,
                call: {AFlexClient,m_masterPubKey,m_tradingPairCode,m_xchgPairCode}
            });
            tvm.sendrawmsg(deployMsg, 1);
        }else
            Terminal.print(0,"Terminated!");
    }

    function onSuccessFCDeployed() public {
        Terminal.print(tvm.functionId(flexClientSetStockCfg), format("Contract deployed {}.\nNow we need to set config..",m_addr[ADDR_FLEX_CLIENT]));
    }

    function onDeployFCFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(deployFlexClientCode), "Do you want to retry?");
    }

    function flexClientSetStockCfg() public view {
        optional(uint256) none;
        TvmCell sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_addr[ADDR_FLEX_CLIENT],
            callbackId: tvm.functionId(onSuccessFCSetTonsCfg),
            onErrorId: tvm.functionId(onFCSetTonsCfgError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.setFlexCfg,m_flexTonCfg,m_addr[ADDR_FLEX]}
        });
        tvm.sendrawmsg(sendMsg, 1);
    }

    function onSuccessFCSetTonsCfg() public {
        Terminal.print(0, "Success!");
        this.hasFlexWalletCode(false);
    }
    function onGetFCWalletCode() public view {
        optional(uint256) none;
        AFlexClient(m_addr[ADDR_FLEX_CLIENT]).hasFlexWalletCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(hasFlexWalletCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function hasFlexWalletCode(bool value0) public {
        if (value0) {
            onGetFCAddressAndKeys();
        }else{
            Terminal.print(tvm.functionId(flexClientSetWalletCode), "We need to set wallet code for you flex client..");
        }
    }

    function flexClientSetWalletCode() public view {
        optional(uint256) none;
        TvmCell sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_addr[ADDR_FLEX_CLIENT],
            callbackId: tvm.functionId(onSuccessFCSetFlexWalletCode),
            onErrorId: tvm.functionId(onFCSetFlexWalletCodeError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.setFlexWalletCode,m_flexWalletCode}
        });
        tvm.sendrawmsg(sendMsg, 1);
    }

    function onSuccessFCSetFlexWalletCode() public {
        Terminal.print(tvm.functionId(onGetFCAddressAndKeys), "Success!");
    }

    function onFCSetFlexWalletCodeError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setFlexWalletCode failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(flexClientSetWalletCode), "Retrying..");
    }

    function onFCSetTonsCfgError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setTonsCfg failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(flexClientSetStockCfg), "Retrying..");
    }

    function checkFlexClientStockCfg() public view {
        optional(uint256) none;
        AFlexClient(m_addr[ADDR_FLEX_CLIENT]).getFlex{
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
        if (value0!=m_addr[ADDR_FLEX]){
            Terminal.print(tvm.functionId(flexClientSetStockCfg), "We need to set config for you flex client..");
        }else{
            onGetFCWalletCode();
        }
    }

    function onGetFCAddressAndKeys() public {
        IFlexDebot(m_addr[ADDR_SENDER]).onGetFCAddressAndKeys(m_addr[ADDR_FLEX_CLIENT],m_signingBox,m_flexDealsLimit,m_flexTonCfg,m_tradingPairCode,m_xchgPairCode);
        m_addr[ADDR_SENDER] = address(0);
    }

//get tip3 wallet address
    function getTip3WalletAddress(address flex, uint32 signingBox, address tip3root, address flexClient, FlexTonCfg tonCfg) public override {
        m_addr[ADDR_FLEX] = flex;
        m_signingBox = signingBox;
        m_addr[ADDR_TIP3_ROOT] = tip3root;
        m_addr[ADDR_FLEX_CLIENT] = flexClient;
        m_flexTonCfg = tonCfg;
        m_addr[ADDR_SENDER] = msg.sender;

        optional(uint256) none;
        AWrapper(m_addr[ADDR_TIP3_ROOT]).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3WrapperSymbol),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3WrapperSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_t3Symbol = symbol;
        optional(uint256) none;
        AWrapper(m_addr[ADDR_TIP3_ROOT]).getWalletAddress{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3WalletAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(0x0,m_addr[ADDR_FLEX_CLIENT]);
    }

    function setTip3WalletAddress(address value0) public {
        m_addr[ADDR_TIP3_WALLET] = value0;
        Sdk.getAccountType(tvm.functionId(Tip3WalletAccType),m_addr[ADDR_TIP3_WALLET]);
    }

    function Tip3WalletAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            Terminal.print(0,format("You don't have {} tip3 wallet. Let deploy it",m_t3Symbol));
            UserInfo.getAccount(tvm.functionId(enterMsigAddr));
        }else if (acc_type==1){
            onGetTip3WalletAddress();
        } else if (acc_type==2){
            Terminal.print(0, format("{} tip3 wallet {} is frozen.",m_t3Symbol,m_addr[ADDR_FLEX_CLIENT]));
        }
    }

    function enterMsigAddr(address value) public {
        m_addr[ADDR_MSIG] = value;
        Terminal.print(0,format("TONs will be transfered from your multisig wallet {}",m_addr[ADDR_MSIG]));
        Sdk.getAccountType(tvm.functionId(getDeplotT3WClientAccType), m_addr[ADDR_MSIG]);
    }

    function getDeplotT3WClientAccType(int8 acc_type) public {
        if (acc_type==1) {
            Sdk.getBalance(tvm.functionId(getDeplotT3WClientBalance), m_addr[ADDR_MSIG]);
        }else {
            Terminal.print(0, "Error: Address attached to your account not active!");
        }
    }

    function getDeplotT3WClientBalance(uint128 nanotokens) public {
        if (nanotokens>3 ton) {
            AmountInput.get(tvm.functionId(enterMsigTons), "How many tons you want to send to FlexClient for trade operations (1 additional ton will transafered for execution fee, not used part will be returned):",9,1 ton,nanotokens-1 ton);
        } else {
            Terminal.print(0, "Error: Your msig balance should be more than 3 TONs!");
        }
    }

    function enterMsigTons(uint128 value) public {
        m_dealTons = value;
        deployEmptyWallet();
    }

    function deployEmptyWallet() view public {
        TvmCell payload = tvm.encodeBody(AWrapper.deployEmptyWallet, 0, 0x0, m_addr[ADDR_FLEX_CLIENT], m_dealTons);
        optional(uint256) none;
        uint128 amount = m_dealTons+1000000000;
        AMSig(m_addr[ADDR_MSIG]).submitTransaction{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onDEWSuccess),
                onErrorId: tvm.functionId(onDEWError),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none
            }(m_addr[ADDR_TIP3_ROOT], amount, true, false, payload);
    }

    function onDEWSuccess (uint64 transId) public pure {
        transId;//disable compile warning
        this.checkT3WWait(true);
    }

    function onDEWError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Empty wallet deploy failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        Terminal.print(tvm.functionId(deployEmptyWallet), "Retrying..");
    }

    function checkT3WWait(bool value) public {
        if (value)
            Sdk.getAccountType(tvm.functionId(Tip3WalletAccTypeWait),m_addr[ADDR_TIP3_WALLET]);
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
        IFlexDebot(m_addr[ADDR_SENDER]).onGetTip3WalletAddress(m_addr[ADDR_TIP3_WALLET]);
        m_addr[ADDR_SENDER] = address(0);
    }


//withdraw tons
    function withdrawTons(address fclient, uint32 signingBox) public override {
        m_addr[ADDR_FLEX_CLIENT] = fclient;
        m_signingBox = signingBox;
        m_addr[ADDR_SENDER] = msg.sender;
        Sdk.getBalance(tvm.functionId(getFlexClientBalance), m_addr[ADDR_FLEX_CLIENT]);
    }

    function getFlexClientBalance(uint128 nanotokens) public {
        Terminal.print(0,format("Your flex client address balance is {:t}",nanotokens));
        if (nanotokens>1 ton) {
            AmountInput.get(tvm.functionId(enterWithdrawalTons),"Please enter amount of tons you want to withdraw",9,1,nanotokens-1 ton);
        } else {
            Terminal.print(tvm.functionId(onWithdrawTons), "Error: Your flex client balance should be more than 1 TON!");
        }
    }
    function enterWithdrawalTons(uint128 value) public {
        m_dealTons = value;
        AddressInput.get(tvm.functionId(enterWithdrawalAddr),"Please enter your withdrawal address");
    }

    function enterWithdrawalAddr(address value) public {
        m_addr[ADDR_MSIG] = value;
        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
                abiVer: 2,
                dest: m_addr[ADDR_FLEX_CLIENT],
                callbackId: tvm.functionId(onWithdrawSuccess),
                onErrorId: tvm.functionId(onWithdrawError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                signBoxHandle: m_signingBox,
                call: {AFlexClient.transfer, m_addr[ADDR_MSIG], m_dealTons, true}
            });

        ConfirmInput.get(tvm.functionId(confirmWithdrawTons),format("Do you want to transfer {:t} tons from your flex client to address {}?",m_dealTons,m_addr[ADDR_MSIG]));
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
        IFlexDebot(m_addr[ADDR_SENDER]).updateTradingPairs();
        m_addr[ADDR_SENDER] = address(0);
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Helper Debot";
        version = "0.2.2";
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
