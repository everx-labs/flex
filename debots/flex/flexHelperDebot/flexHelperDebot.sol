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
import "../abstract/AWrapperEver.sol";
import "../interface/IFlexDebot.sol";
import "../interface/IFlexHelperDebot.sol";
import "../constants/WrapperInfo.sol";
import "../abstract/AFlexWallet.sol";

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
    uint256 m_masterPubkey;
    uint32 m_signingBox;
    TvmCell m_flexClientCode;
    uint128 m_tradingPairDeploy;
    TvmCell m_sendMsg;
    TvmCell m_xchgPairCode;
    TvmCell m_flexWalletCode;
    uint128 m_dealTons;
    EversConfig m_flexEverCfg;
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

    function setABIBytes(bytes dabi) public {
        require(tvm.pubkey() == msg.pubkey(), 100);
        tvm.accept();
        m_options |= DEBOT_ABI;
        m_debotAbi = dabi;
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
        m_masterPubkey = value;
        UserInfo.getSigningBox(tvm.functionId(getSigningBox));
    }

    function onGetMethodError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Get method error. Sdk error = {}, Error code = {}",sdkError, exitCode));
    }

    function getSigningBox(uint32 handle) public {
        m_signingBox = handle;
        //todo get details from parce acc data
        optional(uint256) none;
        AFlex(m_addr[ADDR_FLEX]).getDetails{
            callbackId: setFlexDetails,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setFlexDetails(
        bool initialized,
        EversConfig ev_cfg,
        ListingConfig listing_cfg,
        TvmCell xchg_pair_code,
        TvmCell wrapper_code,
        TvmCell wrapper_ever_code,
        uint8 deals_limit,
        uint256 unsalted_price_code_hash,
        FlexOwnershipInfo ownership,
        WrapperListingRequestWithPubkey[] wrapper_listing_requests,
        XchgPairListingRequestWithPubkey[] xchg_pair_listing_requests
    ) public {
        initialized;listing_cfg;wrapper_code;ownership;wrapper_listing_requests;xchg_pair_listing_requests;
        m_flexEverCfg = ev_cfg;
        m_flexDealsLimit = deals_limit;
        m_xchgPairCode = xchg_pair_code;

        TvmCell deployState = tvm.insertPubkey(m_flexClientCode, m_masterPubkey);
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
        Terminal.print(0,format("EVERs will be transfered from your multisig wallet {}",m_addr[ADDR_MSIG]));
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
            AmountInput.get(tvm.functionId(getBalanceToSend), "How many evers you want to send to FlexClient for trade operations:",9,1 ever,nanotokens-1 ton);
        } else {
            Terminal.print(0, "Error: Your msig balance should be more than 2 EVERs!");
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
                callbackId: tvm.functionId(onFCGrantSuccess),
                onErrorId: tvm.functionId(onFCGrantError),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none
            }(m_addr[ADDR_FLEX_CLIENT], m_dealTons, false, false, payload).extMsg;
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
            TvmCell image = tvm.insertPubkey(m_flexClientCode, m_masterPubkey);
            optional(uint256) none;
            TvmCell deployMsg = tvm.buildExtMsg({
                dest: m_addr[ADDR_FLEX_CLIENT],
                callbackId: tvm.functionId(onSuccessFCDeployed),
                onErrorId: tvm.functionId(onDeployFCFailed),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none,
                stateInit: image,
                call: {AFlexClient,m_masterPubkey}
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
            dest: m_addr[ADDR_FLEX_CLIENT],
            callbackId: tvm.functionId(onSuccessFCSetTonsCfg),
            onErrorId: tvm.functionId(onFCSetTonsCfgError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.setFlexCfg,m_addr[ADDR_FLEX]}
        });
        tvm.sendrawmsg(sendMsg, 1);
    }

    function onSuccessFCSetTonsCfg() public {
        onGetFCAddressAndKeys();
    }

    function onFCSetTonsCfgError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setCrystalCfg failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(flexClientSetStockCfg), "Retrying..");
    }

    function checkFlexClientStockCfg() public view {
        optional(uint256) none;
        AFlexClient(m_addr[ADDR_FLEX_CLIENT]).getFlex{
            callbackId: tvm.functionId(getFlexFlex),
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function getFlexFlex(address value0) public {
        if (value0!=m_addr[ADDR_FLEX]){
            Terminal.print(tvm.functionId(flexClientSetStockCfg), "We need to set config for you flex client..");
        }else{
            onGetFCAddressAndKeys();
        }
    }

    function onGetFCAddressAndKeys() public {
        IFlexDebot(m_addr[ADDR_SENDER]).onGetFCAddressAndKeys(m_addr[ADDR_FLEX_CLIENT],m_signingBox,m_masterPubkey,m_flexDealsLimit,m_flexEverCfg,m_xchgPairCode);
        m_addr[ADDR_SENDER] = address(0);
    }

//get tip3 wallet address
    function getTip3WalletAddress(uint32 signingBox, uint256 pubkey, address tip3root, address flexClient) public override {
        m_signingBox = signingBox;
        m_masterPubkey = pubkey;
        m_addr[ADDR_TIP3_ROOT] = tip3root;
        m_addr[ADDR_FLEX_CLIENT] = flexClient;
        m_addr[ADDR_SENDER] = msg.sender;

        Sdk.getAccountCodeHash(tvm.functionId(getTip3WrapperCodeHash),m_addr[ADDR_TIP3_ROOT]);
    }

    function getTip3WrapperCodeHash(uint256 code_hash) public {
        optional(uint256) none;
        if (code_hash == WrapperInfo.TIP3_WRAPPER_CH){
            AWrapper(m_addr[ADDR_TIP3_ROOT]).getDetails{
                callbackId: getTip3WrapperWalletSymbol,
                onErrorId: onGetMethodError,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }().extMsg;
        }else if (code_hash == WrapperInfo.EVER_WRAPPER_CH){
            AWrapperEver(m_addr[ADDR_TIP3_ROOT]).getDetails{
                callbackId: getEverWrapperWalletSymbol,
                onErrorId: onGetMethodError,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }().extMsg;
        }else {
            Terminal.print(0,"ERROR symbol unknown!");
        }
    }

    function getTip3WrapperWalletSymbol(
        bytes name,
        bytes symbol,
        uint8 decimals,
        uint256 root_pubkey,
        uint128 total_granted,
        TvmCell wallet_code,
        address external_wallet,
        address reserve_wallet,
        address flex)
    public {
        m_t3Symbol = symbol;
        m_addr[ADDR_FLEX] = flex;
        setTip3WrapperSymbol();
    }

    function getEverWrapperWalletSymbol(
        bytes name,
        bytes symbol,
        uint8 decimals,
        uint256 root_pubkey,
        uint128 total_granted,
        TvmCell wallet_code,
        address reserve_wallet,
        address flex)
    public {
        m_t3Symbol = symbol;
        m_addr[ADDR_FLEX] = flex;
        setTip3WrapperSymbol();
    }

    function setTip3WrapperSymbol() public {
        optional(address) opt;
        opt.set(m_addr[ADDR_FLEX_CLIENT]);
        optional(uint256) none;
        AWrapper(m_addr[ADDR_TIP3_ROOT]).getWalletAddress{
            callbackId: tvm.functionId(setTip3WalletAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_masterPubkey,opt).extMsg;
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
            this.isWalletRestrictionSet();
        } else if (acc_type==2){
            Terminal.print(0, format("{} tip3 wallet {} is frozen.",m_t3Symbol,m_addr[ADDR_FLEX_CLIENT]));
        }
    }

    function enterMsigAddr(address value) public {
        m_addr[ADDR_MSIG] = value;
        Terminal.print(0,format("EVERs will be transfered from your multisig wallet {}",m_addr[ADDR_MSIG]));
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
            AmountInput.get(tvm.functionId(enterMsigTons), "How many evers you want to send to FlexWallet for trade operations (1 additional ton will transafered for execution fee, not used part will be returned):",9,1 ton,nanotokens-1 ton);
        } else {
            Terminal.print(0, "Error: Your msig balance should be more than 3 EVER!");
        }
    }

    function enterMsigTons(uint128 value) public {
        m_dealTons = value;
        deployEmptyWallet();
    }

    function deployEmptyWallet() view public {
        TvmCell payload = tvm.encodeBody(AWrapper.deployEmptyWallet, 0, m_masterPubkey, m_addr[ADDR_FLEX_CLIENT], m_dealTons);
        optional(uint256) none;
        uint128 amount = m_dealTons+1000000000;
        AMSig(m_addr[ADDR_MSIG]).submitTransaction{
                callbackId: tvm.functionId(onDEWSuccess),
                onErrorId: tvm.functionId(onDEWError),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none
            }(m_addr[ADDR_TIP3_ROOT], amount, true, false, payload).extMsg;
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
            getUnsaltedPriceCodeHash();
        } else {
            Terminal.print(0, "Unexpected error.");
        }
    }

    function isWalletRestrictionSet() public {
        optional(uint256) none;
        AFlexWallet(m_addr[ADDR_TIP3_WALLET]).getDetails{
            callbackId: getWalletRestrictions,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function getWalletRestrictions(
        string name, string symbol, uint8 decimals, uint128 balance,
        uint256 root_public_key, address root_address, uint256 wallet_public_key,
        address owner_address,
        optional(lend_pubkey_array_record) lend_pubkey,
        lend_owner_array_record[] lend_owners,
        optional(restriction_info) restriction,
        uint128 lend_balance,
        uint256 code_hash,
        uint16 code_depth,
        Allowance allowance,
        int8 workchain_id
    ) public {
        if (restriction.hasValue()&&restriction.get().flex==m_addr[ADDR_FLEX]) {
            this.onGetTip3WalletAddress();
        }else{
            this.getUnsaltedPriceCodeHash();
        }
    }

    function getUnsaltedPriceCodeHash() public {
        optional(uint256) none;
        AFlex(m_addr[ADDR_FLEX]).getDetails{
            callbackId: setUnsaltedPriceCodeHash,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setUnsaltedPriceCodeHash(
        bool initialized,
        EversConfig ev_cfg,
        ListingConfig listing_cfg,
        TvmCell xchg_pair_code,
        TvmCell wrapper_code,
        TvmCell wrapper_ever_code,
        uint8 deals_limit,
        uint256 unsalted_price_code_hash,
        FlexOwnershipInfo ownership,
        WrapperListingRequestWithPubkey[] wrapper_listing_requests,
        XchgPairListingRequestWithPubkey[] xchg_pair_listing_requests
    ) public {
        optional(uint256) none;

        AFlexClient(m_addr[ADDR_FLEX_CLIENT]).setTradeRestriction{
                callbackId: tvm.functionId(onSetTradeRestrictionSuccess),
                onErrorId: tvm.functionId(onSetTradeRestrictionError),
                time: 0,
                expire: 0,
                sign: true,
                signBoxHandle: m_signingBox,
                pubkey: none
            }(0.5 ever, m_addr[ADDR_TIP3_WALLET], m_addr[ADDR_FLEX], unsalted_price_code_hash).extMsg;
    }

    function onSetTradeRestrictionSuccess () public pure {
        this.onGetTip3WalletAddress();
    }

    function onSetTradeRestrictionError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("ERROR setTradeRestriction failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
    }

    function onGetTip3WalletAddress() public {
        IFlexDebot(m_addr[ADDR_SENDER]).onGetTip3WalletAddress(m_addr[ADDR_TIP3_WALLET]);
        m_addr[ADDR_SENDER] = address(0);
    }

//get wrapper symbol
    function getWrapperSymbol(address wrapper) public override {
        m_addr[ADDR_TIP3_ROOT] = wrapper;
        m_addr[ADDR_SENDER] = msg.sender;
        Sdk.getAccountCodeHash(tvm.functionId(getWrapperCodeHash),m_addr[ADDR_TIP3_ROOT]);
    }

    function getWrapperCodeHash(uint256 code_hash) public {
        optional(uint256) none;
        if (code_hash == WrapperInfo.TIP3_WRAPPER_CH){
            AWrapper(m_addr[ADDR_TIP3_ROOT]).getDetails{
                callbackId: getTip3WrapperSymbol,
                onErrorId: onGetMethodError,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }().extMsg;
        }else if (code_hash == WrapperInfo.EVER_WRAPPER_CH){
            AWrapperEver(m_addr[ADDR_TIP3_ROOT]).getDetails{
                callbackId: getEverWrapperSymbol,
                onErrorId: onGetMethodError,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }().extMsg;
        }else {
            IFlexDebot(m_addr[ADDR_SENDER]).onGetWrapperSymbol("---");
        }
    }

    function getTip3WrapperSymbol(
        bytes name,
        bytes symbol,
        uint8 decimals,
        uint256 root_pubkey,
        uint128 total_granted,
        TvmCell wallet_code,
        address external_wallet,
        address reserve_wallet,
        address flex)
    public {
        IFlexDebot(m_addr[ADDR_SENDER]).onGetWrapperSymbol(symbol);
    }

    function getEverWrapperSymbol(
        bytes name,
        bytes symbol,
        uint8 decimals,
        uint256 root_pubkey,
        uint128 total_granted,
        TvmCell wallet_code,
        address reserve_wallet,
        address flex)
    public {
        IFlexDebot(m_addr[ADDR_SENDER]).onGetWrapperSymbol(symbol);
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Helper Debot";
        version = "0.4.0";
        publisher = "";
        key = "Helps Flex to calculate addresses and collect data";
        author = "";
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
