pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "../interfaces/Debot.sol";
import "../interfaces/Upgradable.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/Menu.sol";
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
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
    function getXchgPriceCode(address tip3_addr1, address tip3_addr2) public returns(TvmCell value0) {}
}

abstract contract ATradingPair {
    function getTip3Root() public returns(address value0) {}
    function getFLeXAddr() public returns(address value0) {}
}

abstract contract AXchgPair {
    function getTip3MajorRoot() public returns(address value0) {}
    function getTip3MinorRoot() public returns(address value0) {}
    function getFLeXAddr() public returns(address value0) {}
}

abstract contract ATip3Root {
    function getSymbol() public returns(bytes value0){}
 }

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code) public {}
    function deployPriceWithBuy(TvmCell args_cl) public returns(address value0) {}
    function deployPriceWithSell(TvmCell args_cl) public returns(address value0) {}
    function cancelBuyOrder(TvmCell args_cl) public {}
    function cancelSellOrder(TvmCell args_cl) public {}
    function deployPriceXchg(TvmCell args_cl) public returns(address value0) {}
    function cancelXchgOrder(TvmCell args) public {}
}

interface IFlexHelperDebot {
    function getFCAddressAndKeys(address stock) external;
    function getTip3WalletAddress(address stock, address tip3root, address flexClient, StockTonCfg tonCfg) external;
    function deployXchgTradigPair(address stock, address fclient, uint128 tpd, uint256 pk) external;
    function deployTradigPair(address stock, address fclient, uint128 tpd, uint256 pk) external;
    function withdrawTons(address fclient) external;
}

interface IFlexTip3Tip3Debot {
    function tradeTip3Tip3(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, address stock, address t3major, address t3minor, address wt3major, address wt3minor) external;
}

interface IFlexTip3TonDebot {
    function tradeTip3Ton(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, address stock, address t3root, address wt3) external;    
}

struct PriceInfo {
       uint128 sell;
       uint128 buy;
       address addr;
    }

struct Tip3MenuInfo{
        address tp;
        address t3rMajor;
        address t3rMinor;
        string symbol;
    }

contract FlexDebot is Debot, Upgradable {

    uint256 constant TIP3ROOT_CODEHASH = 0x9e4be18bf6a7f77d5267dde7afa144f905f46a7aa82854df8846ea21f71701c9;

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
    mapping (uint8=>address) m_addr;

    string m_tip3tip3Name;
    address[] m_tpaddrs;
    uint m_curTP;
    Tip3MenuInfo[] m_tip3Menu;
    uint256 m_masterPubKey;
    bool m_bTip3Tip3;
    TvmCell m_pairCode;
    TvmCell m_xchgPairCode;
    TvmCell m_tradingPairCode;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    StockTonCfg m_stockTonCfg;
    bool m_bMajorWallet;
    bytes m_icon;

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
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getFCAddressAndKeys(m_addr[ADDR_FLEX]);
        //callback onGetFCAddressAndKeys
    }

    function onGetFCAddressAndKeys(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, TvmCell tpcode, TvmCell xchgpcode) public{
        m_addr[ADDR_FLEX_CLIENT] = fc;
        m_masterPubKey = pk;
        m_stockMinAmount = minAmount;
        m_stockDealsLimit = dealsLimit;
        m_stockTonCfg = tonCfg;
        m_tradingPairCode = tpcode;
        m_xchgPairCode = xchgpcode;
        
        startMenu(0);        
    }

    function startMenu(uint32 index) public {
        MenuItem[] mi;
        mi.push(MenuItem("Trade TIP3 for TON","",tvm.functionId(menuTradeTip3ForTon)));
        mi.push(MenuItem("Trade TIP3 for TIP3","",tvm.functionId(menuTradeTip3ForTip)));
        Menu.select("Action list:","",mi);
    }

    function menuTradeTip3ForTon(uint32 index) public
    {
        m_bTip3Tip3 = false;
        m_pairCode = m_tradingPairCode;
        m_addr[ADDR_TIP3_MINOR_ROOT] = address(0x0);
        updateTradingPairs();
    }

    function menuTradeTip3ForTip(uint32 index) public
    {
        m_bTip3Tip3 = true;
        m_pairCode = m_xchgPairCode;
        updateTradingPairs();        
    }

    function updateTradingPairs() public {
        m_tpaddrs = new address[](0);
        m_tip3Menu = new  Tip3MenuInfo[](0);
        Sdk.getAccountsDataByHash(tvm.functionId(onAccountsByHash),tvm.hash(m_pairCode),address.makeAddrStd(-1, 0));        
    }

    function onAccountsByHash(ISdk.AccData[] accounts) public {
        
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
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getFLeXAddr{
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
        if (code_hash == TIP3ROOT_CODEHASH){
            getTip3MajorRootSymbol();
        }else{
            getNextTP();
        }
    }

    function getTip3MajorRootSymbol() public view {
        optional(uint256) none;
        ATip3Root(m_addr[ADDR_TIP3_MAJOR_ROOT]).getSymbol{
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

    function setTip3MajorRootSymbol(bytes value0) public {
        m_tip3tip3Name = value0;
        
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
        if (code_hash == TIP3ROOT_CODEHASH){
            getTip3MinorRootSymbol();
        }else{
            getNextTP();
        }
    }

    function getTip3MinorRootSymbol() public view {
        optional(uint256) none;
        ATip3Root(m_addr[ADDR_TIP3_MINOR_ROOT]).getSymbol{
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

    function setTip3MinorRootSymbol(bytes value0) public {
        m_tip3tip3Name.append("/");
        m_tip3tip3Name.append(value0);
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
        mi.push(MenuItem("Deploy trading pair","",tvm.functionId(menuDeployTradigPair)));
        mi.push(MenuItem("Update list","",tvm.functionId(menuUpdateTradigPair)));
        mi.push(MenuItem("Withdraw tons","",tvm.functionId(menuWithdrawTons)));
        mi.push(MenuItem("Back","",tvm.functionId(startMenu)));
        Menu.select("Tip3 list","",mi);
    }

    // Withdraw tons
    function menuWithdrawTons(uint32 index) public
    {
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).withdrawTons(m_addr[ADDR_FLEX_CLIENT]);
        //response to getTradingPairCodeHash
    }

    //tp update
    function menuUpdateTradigPair(uint32 index) public {
        updateTradingPairs();
    }

    // deploy trading pair
    function menuDeployTradigPair(uint32 index) public
    {
        if (m_bTip3Tip3){
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).deployXchgTradigPair(m_addr[ADDR_FLEX],m_addr[ADDR_FLEX_CLIENT],m_stockTonCfg.trading_pair_deploy,m_masterPubKey);
        }else{
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).deployTradigPair(m_addr[ADDR_FLEX],m_addr[ADDR_FLEX_CLIENT],m_stockTonCfg.trading_pair_deploy,m_masterPubKey);
        }
        //response to getTradingPairCodeHash
    }

    //find or deploy tip3 wallet
    function onSelectSymbolMenu(uint32 index) public
    {
        if (!m_bTip3Tip3)
        {
            m_addr[ADDR_TRADING_PAIR] = m_tip3Menu[index].tp;
            m_addr[ADDR_TIP3_MAJOR_ROOT] = m_tip3Menu[index].t3rMajor;
            m_addr[ADDR_TIP3_MINOR_ROOT] = m_tip3Menu[index].t3rMinor;
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_stockTonCfg);
            return;
        }else{        
        m_addr[ADDR_TRADING_PAIR] = m_tip3Menu[index].tp;
        m_addr[ADDR_TIP3_MAJOR_ROOT] = m_tip3Menu[index].t3rMajor;
        m_addr[ADDR_TIP3_MINOR_ROOT] = m_tip3Menu[index].t3rMinor;
        m_bMajorWallet = true;
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_stockTonCfg);
        }
    }

    function onGetTip3WalletAddress(address t3w) public {
        if (!m_bTip3Tip3){
            m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
            IFlexTip3TonDebot(m_addr[ADDR_TIP3TON_DEBOT]).tradeTip3Ton(m_addr[ADDR_FLEX_CLIENT], m_masterPubKey, m_stockMinAmount,m_stockDealsLimit, m_stockTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET]);
        }else{
            if (m_bMajorWallet){
                m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
                m_bMajorWallet =false;
                IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_FLEX_CLIENT], m_stockTonCfg);
            }else{
                m_addr[ADDR_TIP3_MINOR_WALLET] = t3w;
                IFlexTip3Tip3Debot(m_addr[ADDR_TIP3TIP3_DEBOT]).tradeTip3Tip3(m_addr[ADDR_FLEX_CLIENT], m_masterPubKey, m_stockMinAmount,m_stockDealsLimit, m_stockTonCfg, m_addr[ADDR_FLEX], m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_TIP3_MAJOR_WALLET], m_addr[ADDR_TIP3_MINOR_WALLET]);
            }
        }
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Debot";
        version = "0.1.11";
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
