pragma ton-solidity >=0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../interface/IFlexHelperDebot.sol";
import "../interface/IFlexDebot.sol";
import "../interface/IFlexTip3Tip3Debot.sol";
import "../abstract/AWrapper.sol";
import "../abstract/AXchgPair.sol";
import "../abstract/AFlexClient.sol";

    struct Tip3MenuInfo{
        address tp;
        address t3rMajor;
        address t3rMinor;
        string symbol;
    }

contract FlexDebot is IFlexDebot, Debot, Upgradable {

    uint8 constant MAJOR_DETAIL = 0;
    uint8 constant MINOR_DETAIL = 1;

    uint8 constant ADDR_FLEX_HELPER_DEBOT = 0;
    uint8 constant ADDR_TIP3TIP3_DEBOT = 1;
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
    uint256 m_masterPubkey;
    TvmCell m_xchgPairCode;
    uint128 m_flexMinAmount;
    uint8 m_flexDealsLimit;
    EversConfig m_flexCrystalCfg;
    bool m_bMajorWallet;
    bytes m_icon;
    uint128 m_instantPrice;
    uint128 m_instantVolume;
    bool m_instantBuy;
    TvmCell m_sendMsg;
    string[] m_pairNames;

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

    function setABIBytes(bytes dabi) public {
        require(tvm.pubkey() == msg.pubkey(), 100);
        tvm.accept();
        m_options |= DEBOT_ABI;
        m_debotAbi = dabi;
    }

    function start() public override {
        // get FC address, keys and stock data
         m_addr[ADDR_INSTANT_TRADINGPAIR] = address(0);
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getFCAddressAndKeys(m_addr[ADDR_FLEX]);
        //callback onGetFCAddressAndKeys
    }

    function onGetMethodError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Get method error. Sdk error = {}, Error code = {}",sdkError, exitCode));
    }

    function onGetFCAddressAndKeys(address fc, uint32 signingBox, uint256 pubkey, uint8 dealsLimit, EversConfig crystalCfg, TvmCell xchgpcode) public override {
        m_addr[ADDR_FLEX_CLIENT] = fc;
        m_signingBox = signingBox;
        m_flexDealsLimit = dealsLimit;
        m_flexCrystalCfg = crystalCfg;
        m_xchgPairCode = xchgpcode;
        m_masterPubkey = pubkey;
        updateTradingPairs();
    }

    function updateTradingPairs() public override {
        m_tpaddrs = new address[](0);
        m_tip3Menu = new  Tip3MenuInfo[](0);
        Sdk.getAccountsDataByHash(tvm.functionId(onAccountsByHash),tvm.hash(m_xchgPairCode),address.makeAddrStd(-1, 0));
    }

    function onAccountsByHash(AccData[] accounts) public {

        for (uint i=0; i<accounts.length;i++)
        {
            m_tpaddrs.push(accounts[i].id);
        }

        if (accounts.length != 0) {
            Sdk.getAccountsDataByHash(
                tvm.functionId(onAccountsByHash),
                tvm.hash(m_xchgPairCode),
                accounts[accounts.length - 1].id
            );
        } else {
            if (m_tpaddrs.length>0)
            {
                m_curTP = 0;
                getTradingPairInfo(m_tpaddrs[m_curTP]);
            }
            else
                Terminal.print(tvm.functionId(showTip3Menu),"no trading pairs!");
        }
    }

    function getTradingPairInfo(address tpa) public {
        m_addr[ADDR_TRADING_PAIR] = tpa;
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MajorRoot{
            callbackId: tvm.functionId(setTip3MajorRoot),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function getNextTP() public {
        m_curTP += 1;
        if (m_curTP<m_tpaddrs.length)
            getTradingPairInfo(m_tpaddrs[m_curTP]);
        else
            showTip3Menu();
    }

    function setTip3MajorRoot(address value0) public {
        m_addr[ADDR_TIP3_MAJOR_ROOT] = value0;
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getTip3MinorRoot{
            callbackId: tvm.functionId(setTip3MinorRoot),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setTip3MinorRoot(address value0) public {
        m_addr[ADDR_TIP3_MINOR_ROOT] = value0;
        //Terminal.print(0,format("[DEBUG] ADDR_TIP3_MINOR_ROOT {}",value0));

         m_bMajorWallet = true;
         //m_tip3tip3Name = "";
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getWrapperSymbol(m_addr[ADDR_TIP3_MAJOR_ROOT]);
    }

    function onGetWrapperSymbol(string symbol) public override {
        if (m_bMajorWallet) {
            m_bMajorWallet= false;
            m_tip3tip3Name = symbol;
            IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getWrapperSymbol(m_addr[ADDR_TIP3_MINOR_ROOT]);
        } else {
            m_tip3tip3Name.append("/");
            m_tip3tip3Name.append(symbol);
            m_tip3Menu.push(Tip3MenuInfo(m_addr[ADDR_TRADING_PAIR],m_addr[ADDR_TIP3_MAJOR_ROOT],m_addr[ADDR_TIP3_MINOR_ROOT],m_tip3tip3Name));
            getNextTP();
        }
    }

    function showTip3Menu() public {
        MenuItem[] mi;
        for (uint i=0; i<m_tip3Menu.length;i++)
        {
            mi.push( MenuItem(m_tip3Menu[i].symbol,"",tvm.functionId(onSelectPairMenu)));
        }
        mi.push( MenuItem("Reset config","",tvm.functionId(flexClientResetCfg)));
        Menu.select("Trading pairs list","",mi);
    }

    function flexClientResetCfg(uint32 index) public view {
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
            call: {AFlexClient.setFlexCfg,address(0)}
        });
        tvm.sendrawmsg(sendMsg, 1);
    }

    function onSuccessFCSetTonsCfg() public {
         Terminal.print(tvm.functionId(showTip3Menu), "Done");
    }

    function onFCSetTonsCfgError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("setCrystalCfg failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
    }

    function onSelectPairMenu(uint32 index) public {
        m_addr[ADDR_TRADING_PAIR] = m_tip3Menu[index].tp;
        m_addr[ADDR_TIP3_MAJOR_ROOT] = m_tip3Menu[index].t3rMajor;
        m_addr[ADDR_TIP3_MINOR_ROOT] = m_tip3Menu[index].t3rMinor;
        optional(uint256) none;
        AXchgPair(m_addr[ADDR_TRADING_PAIR]).getMinAmount{
            callbackId: tvm.functionId(setXchgMinAmount),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setXchgMinAmount(uint128 value0) public {
        m_flexMinAmount = value0;

        m_bMajorWallet = true;
        IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_signingBox, m_masterPubkey, m_addr[ADDR_TIP3_MAJOR_ROOT], m_addr[ADDR_FLEX_CLIENT]);
    }

     function onGetTip3WalletAddress(address t3w) public override {
         if (m_bMajorWallet) {
             m_bMajorWallet= false;
             m_addr[ADDR_TIP3_MAJOR_WALLET] = t3w;
             IFlexHelperDebot(m_addr[ADDR_FLEX_HELPER_DEBOT]).getTip3WalletAddress(m_signingBox, m_masterPubkey, m_addr[ADDR_TIP3_MINOR_ROOT], m_addr[ADDR_FLEX_CLIENT]);
         } else {
             m_addr[ADDR_TIP3_MINOR_WALLET] = t3w;
             IFlexTip3Tip3Debot(m_addr[ADDR_TIP3TIP3_DEBOT]).tradeTip3Tip3(m_addr[ADDR_FLEX], m_addr[ADDR_FLEX_CLIENT], m_signingBox, m_masterPubkey, m_flexMinAmount, m_addr[ADDR_TRADING_PAIR], m_addr[ADDR_TIP3_MAJOR_WALLET], m_addr[ADDR_TIP3_MINOR_WALLET]);
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
        version = "0.4.0";
        publisher = "";
        caption = "Work with flex";
        author = "";
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