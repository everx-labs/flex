pragma ton-solidity >=0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../abstract/ATip3Root.sol";
import "../abstract/ATip3Wallet.sol";
import "../abstract/AFlex.sol";
import "../abstract/AFlexClient.sol";
import "../abstract/AMSig.sol";
import "../abstract/ATradingPair.sol";
import "../abstract/AXchgPair.sol";
import "../abstract/AWrapper.sol";
import "../interface/IFlexHelperDebot.sol";
import "../interface/IFlexDebot.sol";
import "../interface/IFlexTip3Tip3Debot.sol";
import "../interface/IFlexTip3TonDebot.sol";
import "../abstract/AFlexWallet.sol";

struct Tip3MenuInfo{
        address tp;
        address t3rMajor;
        address t3rMinor;
        string symbol;
    }

contract FlexDebot is IFlexDebot, Debot, Upgradable {

    uint256 constant TIP3WRAPPER_CODEHASH = 0x86920201c74856975aa9bc7cb07b746209dd36834c35b9fe1aca459933b010ff;

    uint8 constant MAJOR_DETAIL = 0;
    uint8 constant MINOR_DETAIL = 1;

    uint8 constant ADDR_FLEX_HELPER_DEBOT = 0;
    uint8 constant ADDR_TIP3TIP3_DEBOT = 1;
    uint8 constant ADDR_TIP3TON_DEBOT = 2;
    uint8 constant ADDR_FLEX = 3;
    uint8 constant ADDR_TIP3_MAJOR_ROOT = 4;
    uint8 constant ADDR_TIP3_MINOR_ROOT = 5;
    uint8 constant ADDR_TRADING_PAIR = 6;
    uint8 constant ADDR_FLEX_CLIENT = 7;
    uint8 constant ADDR_TIP3_MAJOR_WALLET = 8;
    uint8 constant ADDR_TIP3_MINOR_WALLET = 9;
    uint8 constant ADDR_NOTIFY_TP = 10;
    uint8 constant ADDR_MSIG = 11;
    uint8 constant ADDR_INSTANT_TRADINGPAIR = 12;
    mapping (uint8=>address) m_addr;

    string m_tip3tip3Name;
    address[] m_tpaddrs;
    uint m_curTP;
    Tip3MenuInfo[] m_tip3Menu;
    uint32 m_signingBox;
    bool m_bTip3Tip3;
    TvmCell m_pairCode;
    TvmCell m_xchgPairCode;
    TvmCell m_tradingPairCode;
    uint128 m_flexMinAmount;
    uint8 m_flexDealsLimit;
    FlexTonCfg m_flexTonCfg;
    bool m_bMajorWallet;
    bytes m_icon;
    uint128 m_instantPrice;
    uint128 m_instantVolume;
    bool m_instantBuy;
    uint256 m_pubkey;
    TvmCell m_sendMsg;

    function setIcon(bytes icon) public {
        require(msg.pubkey() == tvm.pubkey(), 100);
        tvm.accept();
        m_icon = icon;
    }

    function setFlexAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_addr[ADDR_FLEX] = addr;
    }

    function setFlexHelperDebot(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_addr[ADDR_FLEX_HELPER_DEBOT] = addr;
    }

    function setTip3Tip3FlexDebot(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_addr[ADDR_TIP3TIP3_DEBOT] = addr;
    }

    function setTip3TonFlexDebot(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_addr[ADDR_TIP3TON_DEBOT] = addr;
    }

    function start() public override {
        // get FC address, keys and stock data
        m_addr[ADDR_INSTANT_TRADINGPAIR] = address(0);
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getFCAddressAndKeys(m_addr[ADDR_FLEX]);
        //callback onGetFCAddressAndKeys
    }

    function onGetFCAddressAndKeys(address fc, uint32 signingBox, uint8 dealsLimit, FlexTonCfg tonCfg, TvmCell tpcode, TvmCell xchgpcode) public override {
        m_addr[ADDR_FLEX_CLIENT] = fc;
        m_signingBox = signingBox;
        m_flexDealsLimit = dealsLimit;
        m_flexTonCfg = tonCfg;
        m_tradingPairCode = tpcode;
        m_xchgPairCode = xchgpcode;
        if (m_addr[ADDR_INSTANT_TRADINGPAIR]!=address(0)) {
            Sdk.getAccountCodeHash(tvm.functionId(getInstantTPCodeHash), m_addr[ADDR_INSTANT_TRADINGPAIR]);
        }else{
            startMenu(0);
        }
  }

    /*instant deal */
    function getInstantTPCodeHash(uint256 code_hash) public {
        if (code_hash == tvm.hash(m_tradingPairCode)){
            m_bTip3Tip3 = false;
            m_addr[ADDR_TRADING_PAIR] = m_addr[ADDR_INSTANT_TRADINGPAIR];
            m_addr[ADDR_TIP3_MAJOR_ROOT] = address(0);
            m_addr[ADDR_TIP3_MINOR_ROOT] = address(0);
            optional(uint256) none;
            ATradingPair(m_addr[ADDR_TRADING_PAIR]).getTip3Root{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setInstantTPTip3Root),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else{
            m_bTip3Tip3 = true;
            m_addr[ADDR_TRADING_PAIR] = m_addr[ADDR_INSTANT_TRADINGPAIR];
            m_addr[ADDR_TIP3_MAJOR_ROOT] = address(0);
            m_addr[ADDR_TIP3_MINOR_ROOT] = address(0);
            optional(uint256) none;
            AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MajorRoot{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setInstantTPTip3MajorRoot),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }

    }

    function setInstantTPTip3Root(address value0) public {
        m_addr[ADDR_TIP3_MAJOR_ROOT] = value0;
        getTPMinAmount();
    }

    function setInstantTPTip3MajorRoot(address value0) public {
        m_addr[ADDR_TIP3_MAJOR_ROOT] = value0;
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setInstantTPTip3MinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setInstantTPTip3MinorRoot(address value0) public {
        m_addr[ADDR_TIP3_MINOR_ROOT] = value0;
        getTPMinAmount();
    }

    function startMenu(uint32 index) public {
        index;//disable compile warning
        MenuItem[] mi;
        mi.push(MenuItem("Trade TIP3 for TON","",tvm.functionId(menuTradeTip3ForTon)));
        mi.push(MenuItem("Trade TIP3 for TIP3","",tvm.functionId(menuTradeTip3ForTip)));
        Menu.select("Action list:","",mi);
    }

    function menuTradeTip3ForTon(uint32 index) public
    {
        index;//disable compile warning
        m_bTip3Tip3 = false;
        m_pairCode = m_tradingPairCode;
        m_addr[ADDR_TIP3_MINOR_ROOT] = address(0x0);
        updateTradingPairs();
    }

    function menuTradeTip3ForTip(uint32 index) public
    {
        index;//disable compile warning
        m_bTip3Tip3 = true;
        m_pairCode = m_xchgPairCode;
        updateTradingPairs();
    }

    function updateTradingPairs() public override {
        m_tpaddrs = new address[](0);
        m_tip3Menu = new  Tip3MenuInfo[](0);
        Sdk.getAccountsDataByHash(tvm.functionId(onAccountsByHash),tvm.hash(m_pairCode),address.makeAddrStd(-1, 0));
    }

    function onAccountsByHash(AccData[] accounts) public {

        for (uint i=0; i<accounts.length;i++)
        {
            m_tpaddrs.push(accounts[i].id);
        }

        if (accounts.length != 0) {
            Sdk.getAccountsDataByHash(
                tvm.functionId(onAccountsByHash),
                tvm.hash(m_pairCode),
                accounts[accounts.length - 1].id
            );
        } else {
            if (m_tpaddrs.length>0)
            {
                m_curTP = 0;
                getTradingPairStock(m_tpaddrs[m_curTP]);
            }
            else
                Terminal.print(tvm.functionId(showTip3Menu),"no trading pairs!");
        }
    }

    function getTradingPairStock(address tpa) public {
        m_addr[ADDR_TRADING_PAIR] = tpa;
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getFlexAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTradingPairStock),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTradingPairStock(address value0) public {
        if (value0 == m_addr[ADDR_FLEX])
        {
            getTradingPairTip3MajorRoot();
        }else
        {
            getNextTP();
        }
    }

    function getTradingPairTip3MajorRoot() public view {
        optional(uint256) none;
        if (m_bTip3Tip3)
        {
            AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MajorRoot{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setTip3MajorRoot),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else{
            ATradingPair(m_addr[ADDR_TRADING_PAIR]).getTip3Root{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setTip3MajorRoot),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }
    }

    function setTip3MajorRoot(address value0) public {
        m_addr[ADDR_TIP3_MAJOR_ROOT] = value0;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3MajorRootCodeHash), m_addr[ADDR_TIP3_MAJOR_ROOT]);
    }

    function getTip3MajorRootCodeHash(uint256 code_hash) public {
        if (code_hash == TIP3WRAPPER_CODEHASH){
            getTip3MajorRootSymbol();
        }else{
            getNextTP();
        }
    }

    function getTip3MajorRootSymbol() public view {
        optional(uint256) none;
        AWrapper(m_addr[ADDR_TIP3_MAJOR_ROOT]).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3MajorRootSymbol),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3MajorRootSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3tip3Name = symbol;
        if (m_bTip3Tip3)
        {
            getTradingPairTip3MinorRoot();
        }else {
            addAndNextTradingPair();
        }
    }

    //minor tip3
    function getTradingPairTip3MinorRoot() public view {
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3MinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3MinorRoot(address value0) public {
        m_addr[ADDR_TIP3_MINOR_ROOT] = value0;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3MinorRootCodeHash), m_addr[ADDR_TIP3_MINOR_ROOT]);
    }

    function getTip3MinorRootCodeHash(uint256 code_hash) public {
        if (code_hash == TIP3WRAPPER_CODEHASH){
            getTip3MinorRootSymbol();
        }else{
            getNextTP();
        }
    }

    function getTip3MinorRootSymbol() public view {
        optional(uint256) none;
        AWrapper(m_addr[ADDR_TIP3_MINOR_ROOT]).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3MinorRootSymbol),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3MinorRootSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3tip3Name.append("/");
        m_tip3tip3Name.append(symbol);
        addAndNextTradingPair();
    }

    function addAndNextTradingPair() public {
        m_tip3Menu.push(Tip3MenuInfo(m_addr[ADDR_TRADING_PAIR],m_addr[ADDR_TIP3_MAJOR_ROOT],m_addr[ADDR_TIP3_MINOR_ROOT],m_tip3tip3Name));
        getNextTP();
    }
    //end minor tip3

    function getNextTP() public {
        m_curTP += 1;
        if (m_curTP<m_tpaddrs.length)
            getTradingPairStock(m_tpaddrs[m_curTP]);
        else
            showTip3Menu();
    }

    function showTip3Menu() public {
        MenuItem[] mi;
        for (uint i=0; i<m_tip3Menu.length;i++)
        {
            mi.push( MenuItem(m_tip3Menu[i].symbol,"",tvm.functionId(onSelectSymbolMenu)));
        }
        mi.push(MenuItem("Update list","",tvm.functionId(menuUpdateTradigPair)));
        mi.push(MenuItem("Withdraw tons","",tvm.functionId(menuWithdrawTons)));
        mi.push(MenuItem("Back","",tvm.functionId(startMenu)));
        Menu.select("Tip3 list","",mi);
    }

    // Withdraw tons
    function menuWithdrawTons(uint32 index) public view {
        index;//disable compile warning
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).withdrawTons(m_addr[ADDR_FLEX_CLIENT],m_signingBox);
        //response to getTradingPairCodeHash
    }

    //tp update
    function menuUpdateTradigPair(uint32 index) public {
        index;//disable compile warning
        updateTradingPairs();
    }

    //find or deploy tip3 wallet
    function onSelectSymbolMenu(uint32 index) public {
        m_addr[ADDR_TRADING_PAIR] = m_tip3Menu[index].tp;
        m_addr[ADDR_TIP3_MAJOR_ROOT] = m_tip3Menu[index].t3rMajor;
        m_addr[ADDR_TIP3_MINOR_ROOT] = m_tip3Menu[index].t3rMinor;
        getTPMinAmount();
    }

    function getTPMinAmount() public view {
        optional(uint256) none;
        if (!m_bTip3Tip3){
            ATradingPair(m_addr[ADDR_TRADING_PAIR]).getMinAmount{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onSetTradingPairMinAmount),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else {
            AXchgPair(m_addr[ADDR_TRADING_PAIR]).getMinAmount{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onSetTradingPairMinAmount),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }
    }

    function onSetTradingPairMinAmount(uint128 value0) public
    {
        m_flexMinAmount = value0;
        optional(uint256) none;
        if (!m_bTip3Tip3){
            ATradingPair(m_addr[ADDR_TRADING_PAIR]).getNotifyAddr{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onSetTradingPairNotifyAddress),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else {
            AXchgPair(m_addr[ADDR_TRADING_PAIR]).getNotifyAddr{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onSetTradingPairNotifyAddress),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }
    }

    function onSetTradingPairNotifyAddress(address value0) public
    {
        m_addr[ADDR_NOTIFY_TP] = value0;
        if (!m_bTip3Tip3)
        {
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX], m_signingBox,  m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_flexTonCfg);
            return;
        }else{
            m_bMajorWallet = true;
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX],  m_signingBox, m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_flexTonCfg);
        }
    }

    function onGetTip3WalletAddress(address t3w) public override {
        if (m_addr[ADDR_INSTANT_TRADINGPAIR]==address(0)){
            if (!m_bTip3Tip3){
                m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
                IFlexTip3TonDebot(m_addr[ADDR_TIP3TON_DEBOT]).tradeTip3Ton(m_addr[ADDR_FLEX_CLIENT], m_signingBox, m_addr[ADDR_NOTIFY_TP], m_flexMinAmount,m_flexDealsLimit, m_flexTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET]);
            }else{
                if (m_bMajorWallet){
                    m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
                    m_bMajorWallet =false;
                    IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX],  m_signingBox, m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_flexTonCfg);
                }else{
                    m_addr[ADDR_TIP3_MINOR_WALLET] = t3w;
                    IFlexTip3Tip3Debot(m_addr[ADDR_TIP3TIP3_DEBOT]).tradeTip3Tip3(m_addr[ADDR_FLEX_CLIENT], m_signingBox, m_addr[ADDR_NOTIFY_TP], m_flexMinAmount,m_flexDealsLimit, m_flexTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET], m_addr[ADDR_TIP3_MINOR_WALLET]);
                }
            }
        }else{
            m_addr[ADDR_INSTANT_TRADINGPAIR]=address(0);
            if (!m_bTip3Tip3){
                m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
                IFlexTip3TonDebot(m_addr[ADDR_TIP3TON_DEBOT]).instantTradeTip3Ton(m_addr[ADDR_FLEX_CLIENT], m_signingBox, m_addr[ADDR_NOTIFY_TP], m_flexMinAmount,m_flexDealsLimit, m_flexTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET],m_instantBuy,m_instantPrice,m_instantVolume);
            }else{
                if (m_bMajorWallet){
                    m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
                    m_bMajorWallet =false;
                    IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX],  m_signingBox, m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_flexTonCfg);
                }else{
                    m_addr[ADDR_TIP3_MINOR_WALLET] = t3w;
                    IFlexTip3Tip3Debot(m_addr[ADDR_TIP3TIP3_DEBOT]).instantTradeTip3Tip3(m_addr[ADDR_FLEX_CLIENT], m_signingBox, m_addr[ADDR_NOTIFY_TP], m_flexMinAmount,m_flexDealsLimit, m_flexTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET], m_addr[ADDR_TIP3_MINOR_WALLET],m_instantBuy,m_instantPrice,m_instantVolume);
                }
            }

        }
    }

    /*
    // withdraw tip3
    */
    function burnTip3Wallet(address fclient, uint32 signingBox, address t3Wallet) public override {
        m_addr[ADDR_FLEX_CLIENT] = fclient;
        m_signingBox = signingBox;
        m_addr[ADDR_TIP3_MAJOR_WALLET] = t3Wallet;
        Sdk.getBalance(tvm.functionId(getBurnTip3WalletFlexClientBalance), m_addr[ADDR_FLEX_CLIENT]);
    }

    function getBurnTip3WalletFlexClientBalance(uint128 nanotokens) public {
        Terminal.print(0,"This operation will burn your tip3 internal wallet and send tip3 tokens to specified external wallet");
        Terminal.print(0,format("Your flex client address balance is {:t}",nanotokens));
        if (nanotokens>1.5 ton) {
            optional(uint256) none;
            AFlexWallet(m_addr[ADDR_TIP3_MAJOR_WALLET]).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getT3WalletDetails),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
            }();
        } else {
            Terminal.print(tvm.functionId(onBurnTip3Wallet), "Error: Your flex client balance should be more than 1.5 TON!");
        }
    }

    function getT3WalletDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;symbol;decimals;root_public_key;root_address;wallet_public_key;owner_address;lend_ownership;lend_balance;code;allowance;workchain_id;//disable compile warnings
        if (lend_balance>0) {
           Terminal.print(tvm.functionId(onBurnTip3Wallet), "Error: Your tip3 wallet shouldn't be lend!");
        } else {
            if (balance>0){
                Terminal.input(tvm.functionId(getExtTip3WalletPublicKey),"Please enter public key attached to external tip3 wallet",false);
            }else {
                Terminal.print(tvm.functionId(onBurnTip3Wallet), "Error: Your tip3 wallet balance is zero!");
            }
        }


    }

    function getExtTip3WalletPublicKey(string value) public {
        uint res;
        bool status;
        (res, status) = stoi("0x"+value);
        if (status && res!=0) {
            m_pubkey = res;
            AddressInput.get(tvm.functionId(getExtTip3WalletOwner),"Enter owneer address attached to external tip3 wallet");
        } else
            Terminal.input(tvm.functionId(getExtTip3WalletPublicKey),"Wrong public key. Try again!\nPlease enter public key attached to external tip3 wallet",false);
    }

    function getExtTip3WalletOwner(address value) public {
        m_addr[ADDR_MSIG] = value;
        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
                abiVer: 2,
                dest: m_addr[ADDR_FLEX_CLIENT],
                callbackId: tvm.functionId(onBurnSuccess),
                onErrorId: tvm.functionId(onBurnError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                signBoxHandle: m_signingBox,
                call: {AFlexClient.burnWallet, 1 ton, m_pubkey, m_addr[ADDR_MSIG], m_addr[ADDR_TIP3_MAJOR_WALLET]}
            });
        ConfirmInput.get(tvm.functionId(confirmBurn),format("Do you want to burn wallet {}?",m_addr[ADDR_TIP3_MAJOR_WALLET]));
    }

    function confirmBurn(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        }else {
            onBurnTip3Wallet();
        }
    }

    function onBurnSuccess ()  public  {
        Terminal.print(tvm.functionId(onBurnTip3Wallet),"Transaction sent!");
    }
    function onBurnError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Burn failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmBurn),"Do you want ot retry?");
    }
    function onBurnTip3Wallet() public {
        updateTradingPairs();
    }

    /*
    // invoke buy
    */
    function invokeWithBuyOrder(address tradingPair, uint128 price, uint128 volume) public override {
        m_addr[ADDR_INSTANT_TRADINGPAIR] = tradingPair;
        m_instantPrice = price;
        m_instantVolume = volume;
        m_instantBuy = true;

        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getFCAddressAndKeys(m_addr[ADDR_FLEX]);
    }

    /*
    // invoke sell
    */
    function invokeWithSellOrder(address tradingPair, uint128 price, uint128 volume) public override {
        m_addr[ADDR_INSTANT_TRADINGPAIR] = tradingPair;
        m_instantPrice = price;
        m_instantVolume = volume;
        m_instantBuy = false;

        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getFCAddressAndKeys(m_addr[ADDR_FLEX]);
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Debot";
        version = "0.3.0";
        publisher = "TON Labs";
        caption = "Work with flex";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, i'am a Flex DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = m_icon;
    }

    function getRequiredInterfaces() public view override returns (uint256[] interfaces) {
        return [ AddressInput.ID, AmountInput.ID, ConfirmInput.ID, Menu.ID, Sdk.ID, Terminal.ID ];
    }

    /*
    *  Implementation of Upgradable
    */
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}