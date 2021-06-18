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
import "../interfaces/AmountInput.sol";
import "../interfaces/ConfirmInput.sol";
import "../interfaces/AddressInput.sol";

struct LendOwnership{
   address owner;
   uint128 lend_balance;
   uint32 lend_finish_time;
}
struct Allowance{
    address spender;
    uint128 remainingTokens;
}

struct TokenInfo{
    address root;
    string name;
    string symbol;
}

abstract contract ATip3Root {
    constructor(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint256 root_owner, uint128 total_supply) public {}
    function getName() public returns(string value0) {}
    function getSymbol() public returns(bytes value0){}
    function setWalletCode(TvmCell wallet_code) public {}
    function deployWallet(uint32 _answer_id, int8 workchain_id, uint256 pubkey, uint256 internal_owner,
        uint128 tokens, uint128 grams) public functionID(1888586564) returns(address value0) {}
    function grant(address dest, uint128 tokens, uint128 grams) public {}

    function getWalletAddress(int8 workchain_id, uint256 pubkey, uint256 owner_std_addr) public returns(address value0) {}
    function getDecimals() public returns (uint8 value0) {}
    function getTotalSupply() public returns (uint128 value0) {}
    function getTotalGranted() public returns (uint128 value0) {}
    function getWalletCode() public returns (TvmCell value0) {}
    function getWalletCodeHash() public returns(uint256 value0) {}
}

abstract contract TIP3Wallet {
    function getDetails() public returns (bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) {}
}

abstract contract Utility {
    function splitTokens(uint128 tokens, uint8 decimals) private pure returns (uint64, uint64) {
        uint128 val = 1;
        for(int i = 0; i < decimals; i++) {
            val *= 10;
        }
        uint64 integer = uint64(tokens / val);
        uint64 float = uint64(tokens - (integer * val));
        return (integer, float);
    }

    function tokensToStr(uint128 tokens, uint8 decimals) public returns (string) {
        (uint64 dec, uint64 float) = splitTokens(tokens, decimals);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }
}

contract Tip3Debot is Debot, Upgradable, Transferable, Utility {
    uint256 m_masterPubKey;
    uint256 m_masterSecKey;

    string m_name;
    string m_symbol;
    uint8 m_decimals;
    uint128 m_totalSupply;
    uint128 m_totalGranted;
    address m_tip3wallet;
    address m_tip3root;
    uint256 m_t3wCodeHash;

    string m_seedphrase;
    uint128 m_rootBalance;
    uint128 m_grantAmount;
    uint128 m_nativeAmount;

    address m_flexClient;

    TvmCell m_t3rContractCode;
    TvmCell m_t3WalletCode;
   //TvmCell m_flexClientCode;

    address m_deployAddress;
    TvmCell m_deployState;

    uint32 m_seedPhraseCallBack;

    TokenInfo[] m_tokenRoots;
    uint m_currentTokenIndex;

    function setT3RCode(TvmCell code) public {
        require(tvm.pubkey() == msg.pubkey(), 100);
        tvm.accept();
        m_t3rContractCode = code;
    }

    function setT3WalletCode(TvmCell code) public {
        require(tvm.pubkey() == msg.pubkey(), 100);
        tvm.accept();
        m_t3WalletCode = code.toSlice().loadRef();
    }

    function start() public override {
        Terminal.print(0, "Hi, I can help you manage TIP3 Tokens.");
        Menu.select("Where do we start?", "", [
            MenuItem("Choose TIP3 token","",tvm.functionId(menuChooseToken)),
            MenuItem("Mint new TIP3 token","",tvm.functionId(menuDeployRoot))
        ]);
    }

    function menuDeployRoot(uint32 index) public {
        Terminal.input(tvm.functionId(inputPublicKey), "Enter public key attached to your account:", false);
    }

    function inputPublicKey(string value) public {
        (uint256 key, bool res) = stoi("0x" + value);
        if (!res) {
            Terminal.print(tvm.functionId(Debot.start), "Invalid public key");
            return;
        }
        m_masterPubKey = key;
        DeployRootStep1();
    }

    function menuChooseToken(uint32 index) public {
        index = index;
        Terminal.print(tvm.functionId(startSearching), "Searching for tokens will take a few seconds. Please, wait.");
    }

    function startSearching() public {
        m_currentTokenIndex = 0;
        m_tokenRoots = new TokenInfo[](0);
        TvmCell t3rCode = m_t3rContractCode.toSlice().loadRef();
        Sdk.getAccountsDataByHash(
            tvm.functionId(onDataResult),
            tvm.hash(t3rCode),
            address.makeAddrStd(-1, 0)
        );
    }

    function onDataResult(ISdk.AccData[] accounts) public {
        optional(uint256) none;
        m_currentTokenIndex = m_tokenRoots.length;
        for (uint i=0; i < accounts.length;i++) {
            m_tokenRoots.push(TokenInfo(accounts[i].id, "", ""));
            ATip3Root(accounts[i].id).getName{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setName),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
            ATip3Root(accounts[i].id).getSymbol{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setSymbol),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }
        if (accounts.length != 0) {
            TvmCell t3rCode = m_t3rContractCode.toSlice().loadRef();
            Sdk.getAccountsDataByHash(
                tvm.functionId(onDataResult),
                tvm.hash(t3rCode),
                accounts[accounts.length - 1].id
            );
        } else {
            m_currentTokenIndex = 0;
            printTokenList(0);
        }
    }

    function printTokenList(uint32 index) public {
        index = index;
        MenuItem[] items;
        for(uint i = 0; i < m_tokenRoots.length; i++) {
            items.push(MenuItem(format("{} ({})", m_tokenRoots[i].name, m_tokenRoots[i].symbol), "", tvm.functionId(menuSetTipRoot)));
        }
        items.push(MenuItem("Select token by address", "", tvm.functionId(menuEnterRootAddress)) );
        Menu.select(format("{} TIP3 tokens found. Choose token from the list or enter its root address manually:", m_tokenRoots.length), "", items);
    }

    function menuEnterRootAddress(uint32 index) public {
        index = index;
        AddressInput.get(tvm.functionId(setTipRoot), "Enter root address:");
    }

    function setName(string value0) public {
        m_tokenRoots[m_currentTokenIndex].name = value0;
    }

    function setSymbol(string value0) public {
        m_tokenRoots[m_currentTokenIndex].symbol = value0;
        m_currentTokenIndex++;
    }
//////////////////////////////////////////////////////////////////////

    function DeployRootStep1() public {
        rnd.setSeed(now);
        rnd.shuffle();
        m_deployState = tvm.insertPubkey(m_t3rContractCode, rnd.next());
        m_deployAddress = address.makeAddrStd(0, tvm.hash(m_deployState));
        Sdk.getAccountType(tvm.functionId(DeployRootStep2), m_deployAddress);
    }
    function DeployRootStep2(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            Terminal.input(tvm.functionId(DeployRootStep3),"Enter TIP3 name:",false);
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is active. Try to use another seed phrase",m_deployAddress));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is frozen. Try to use another seed phrase",m_deployAddress));
        }
    }
    function DeployRootStep3(string value) public {
        m_name = value;
        Terminal.input(tvm.functionId(DeployRootStep4),"Enter TIP3 symbol:",false);
    }
    function DeployRootStep4(string value) public {
        m_symbol = value;
        AmountInput.get(tvm.functionId(DeployRootStep5),"Enter TIP3 decimals:",0,0,3);
    }
    function DeployRootStep5(int256 value) public {
        m_decimals = uint8(value);
        AmountInput.get(tvm.functionId(DeployRootStep6),"Enter TIP3 total supply:",m_decimals,1,0xFFFFFFFFFFFFFFFF);
    }
    function DeployRootStep6(int256 value) public {
        m_totalSupply = uint128(value);
        DeployRootStep7();
    }
    function DeployRootStep7() public {
        Terminal.print(0, "Please, send at least 5 tokens to new TIP3 Root address:");
        Terminal.print(0, format("{}", m_deployAddress));
        ConfirmInput.get(tvm.functionId(DeployRootStep8),format("And then i will activate it.\nDid you send tokens?"));
    }
    function DeployRootStep8(bool value) public {
        if (value){
            Sdk.getAccountType(tvm.functionId(DeployRootStep9), m_deployAddress);
        } else
            Terminal.print(tvm.functionId(Debot.start),"Terminated!");
    }

    function DeployRootStep9(int8 acc_type) public {
        if (acc_type==-1) {
            DeployRootStep7();
        }else if (acc_type==0) {
            Sdk.getBalance(tvm.functionId(DeployRootStep9_getBalance), m_deployAddress);
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is already active.",m_deployAddress));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is frozen.",m_deployAddress));
        }
    }
    function DeployRootStep9_getBalance(uint128 nanotokens) public {
        if (nanotokens<5 ton) {
            Terminal.print(tvm.functionId(DeployRootStep7), format("Account {} balance is {:t}.",m_deployAddress,nanotokens));
        } else {
            Terminal.print(tvm.functionId(DeployRootStep10Proxy), "Initializing your TIP3 Token");
        }
    }

    function DeployRootStep10Proxy() public {
        DeployRootStep10(true);
    }
    function DeployRootStep10(bool value) public {
        if (value) {
            optional(uint256) key = m_masterPubKey;
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_deployAddress,
                callbackId: tvm.functionId(onSuccessDeployed),
                onErrorId: tvm.functionId(onDeployFailed),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: key,
                stateInit: m_deployState,
                call: {ATip3Root,m_name,m_symbol,m_decimals,m_masterPubKey,0x0,m_totalSupply}
            });
            tvm.sendrawmsg(deployMsg, 0);
        }else
            Terminal.print(tvm.functionId(Debot.start),"Terminated!");
    }

    function onSuccessDeployed() public {
        Terminal.print(0, "TIP3 token created.");
        Terminal.print(0, "A few more seconds. I need to upload wallet code.");
        ConfirmInput.get(tvm.functionId(DeployRootStep11), "Please, approve.");
    }

    function onDeployFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep10), "Do you want to retry?");
    }

    function DeployRootStep11(bool value) public {
        if (!value) {

            return;
        }
        optional(uint256) key = m_masterPubKey;
        ATip3Root(m_deployAddress).setWalletCode{
            abiVer: 2,
            extMsg: true,
            sign: true,
            callbackId: tvm.functionId(onT3WCSuccess),
            onErrorId: tvm.functionId(onT3WCError),
            time: 0,
            expire: 0,
            pubkey: key
        }(m_t3WalletCode);
    }

    function onT3WCSuccess() public {
        Terminal.print(0, "Congrats! Now you have your own TIP3 token.\nTIP3 root address:");
        Terminal.print(tvm.functionId(Debot.start), format("{}", m_deployAddress));
    }
    function onT3WCError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep11), "retrying..");
    }

//////////////////////////////////////////////////////////////////////
    function calcTipWallet(string value) public {
        (uint256 key, bool res) = stoi("0x" + value);
        if (!res) {
            menuGrantTokens(0);
            return;
        }
        m_masterPubKey = key;
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
        }(0, m_masterPubKey, 0);
    }

    function menuSetTipRoot(uint32 index) public {
        setTipRoot(m_tokenRoots[index].root);
    }

    function setTipRoot(address value) public {
        m_tip3root = value;
        Sdk.getBalance(tvm.functionId(setRootBalance), m_tip3root);
        Sdk.getAccountType(tvm.functionId(checkRootExistance), m_tip3root);
    }

    function checkRootExistance(int8 acc_type) public {
        if (acc_type == -1 || acc_type == 0 || acc_type == 2) {
            Terminal.print(tvm.functionId(Debot.start),"This TIP3 Root is inactive.");
            return;
        }
        Sdk.getAccountCodeHash(tvm.functionId(checkRootCodeHash), m_tip3root);        
    }

    function checkRootCodeHash(uint256 code_hash) public {
        TvmCell t3rCode = m_t3rContractCode.toSlice().loadRef();
        if(code_hash!=tvm.hash(t3rCode)){
            Terminal.print(tvm.functionId(Debot.start),format("Account {} is not a valid tip3 token.",m_tip3root));
            return;
        }
        getTokenInfo();
        Terminal.print(tvm.functionId(printTokenDetails), "Token Details:");
    }

    function setRootBalance(uint128 nanotokens) public {
        m_rootBalance = nanotokens;
    }

    function setTotalSupply(uint128 value0) public {
        m_totalSupply = value0;
    }

    function setTotalGranted(uint128 value0) public {
        m_totalGranted = value0;
    }

    function setDecimals(uint8 value0) public {
        m_decimals = value0;
    }

    function setTokenName(string value0) public {
        m_name = value0;
    }

    function setWalletCodeHash(uint256 value0) public {
        m_t3wCodeHash = value0;
    }

    function getTokenInfo() public view {
        optional(uint256) none;
        ATip3Root(m_tip3root).getName{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTokenName),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

        ATip3Root(m_tip3root).getDecimals{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setDecimals),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

        ATip3Root(m_tip3root).getTotalSupply{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTotalSupply),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

        ATip3Root(m_tip3root).getTotalGranted{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTotalGranted),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

        ATip3Root(m_tip3root).getWalletCodeHash{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setWalletCodeHash),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function printTokenDetails() public {
        Terminal.print(0, format("Name: {}\nTotal supply: {}\nTotal granted: {}\nRoot address: {}",
            m_name, tokensToStr(m_totalSupply, m_decimals), tokensToStr(m_totalGranted, m_decimals), m_tip3root));

        Menu.select("What's next?", "", [
            MenuItem("Grant tokens", "", tvm.functionId(menuGrantTokens)),
            MenuItem("Back", "", tvm.functionId(printTokenList))
        ]);
    }

    function menuGrantTokens(uint32 index) public {
        index = index;
        Menu.select("Select destination TIP3 wallet:", "", [
            MenuItem("By address", "", tvm.functionId(menuEnterAddress)),
            MenuItem("By public key", "", tvm.functionId(menuCalcAddress))
        ]);
    }

    function menuCalcAddress(uint32 index) public {
        Terminal.input(tvm.functionId(calcTipWallet), "Enter public key attached to your account:", false);
    }

    function menuEnterAddress(uint32 index) public {
        AddressInput.get(tvm.functionId(setAddress), "Enter TIP3 wallet address:");
    }

    function setAddress(address value) public {
        setTip3WalletAddress(value);
    }

    function setTip3WalletAddress(address value0) public {
        m_tip3wallet = value0;
        Terminal.print(0, "TIP3 wallet address:");
        Terminal.print(tvm.functionId(checkWalletBalance), format("{}", value0));
    }

    function setGrantAmount(uint128 value) public {
        m_grantAmount = value;
        m_nativeAmount = 0.2 ton;
        ConfirmInput.get(
            tvm.functionId(checkWallet),
            format("Ok, i'm ready to grant {} TIP3 tokens to the wallet {}. Also {} native tokens will be transfered to the wallet to pay for operation. Confirm and sign.",
            tokensToStr(m_grantAmount, m_decimals), m_tip3wallet, tokensToStr(m_nativeAmount, 9))
        );
    }

    function checkWallet(bool value) public {
        if (!value) {
            start();
            return;
        }
        Sdk.getAccountType(tvm.functionId(checkWalletExistance), m_tip3wallet);
    }

    function startGrant() public {
        AmountInput.get(tvm.functionId(setGrantAmount), "How many TIP3 tokens grant to this wallet?", m_decimals, 1, m_totalSupply - m_totalGranted);
    }

    function checkWalletBalance() public {
        Sdk.getAccountType(tvm.functionId(checkWalletStatus), m_tip3wallet);
    }

    function checkWalletStatus(int8 acc_type) public {
        if (acc_type == 1) {
            Sdk.getAccountCodeHash(tvm.functionId(checkWalletCodeHash), m_tip3wallet);            
        } else {
            Terminal.print(tvm.functionId(Debot.start), format("Error: Account {} is not active!",m_tip3wallet));
        }
    }

    function  checkWalletCodeHash(uint256 code_hash) public {
        TvmCell t3wCode = m_t3WalletCode.toSlice().loadRef();
        Terminal.print(0, format("debug: Account {} code_hash {:064x}",m_tip3wallet,code_hash));
        Terminal.print(0, format("debug: ethalon code_hash {:064x}",m_t3wCodeHash));
        if (code_hash == m_t3wCodeHash){
            optional(uint256) none;
            TIP3Wallet(m_tip3wallet).getDetails{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(printWalletBalance),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        } else {
            Terminal.print(tvm.functionId(Debot.start), format("Error: Account {} is not a valid tip3 wallet!",m_tip3wallet));
        }

    }

    function printWalletBalance(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public {
        Terminal.print(0, format("TIP3 Balance: {} tokens", tokensToStr(balance, m_decimals)));
        startGrant();
    }

    function checkWalletExistance(int8 acc_type) public {
        optional(uint256) pubkey = 0;
        if (acc_type == -1 || acc_type == 0) {
            ATip3Root(m_tip3root).deployWallet{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onDeployWalletSuccess),
                onErrorId: tvm.functionId(onDeployWalletError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: pubkey
            }(0, 0, m_masterPubKey, 0, m_grantAmount, m_nativeAmount);
        } else if (acc_type == 1) {
            ATip3Root(m_tip3root).grant{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(onGrantSuccess),
                onErrorId: tvm.functionId(onGrantError),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: pubkey
            }(m_tip3wallet, m_grantAmount, m_nativeAmount);
        } else {
            Terminal.print(0, "Your wallet is frozen");
        }
    }

    function onDeployWalletSuccess(address value0) public {
        Terminal.print(tvm.functionId(Debot.start), "Succeeded. TIP3 wallet deployed and granted with tokens.");
    }

    function onGrantSuccess() public {
        Terminal.print(tvm.functionId(Debot.start), "Succeeded. TIP3 wallet granted with tokens.");
    }

    function onDeployWalletError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(tvm.functionId(Debot.start), format("Deploy failed: error {}, code {} ", sdkError, exitCode));
    }

    function onGrantError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(tvm.functionId(Debot.start), format("Granting failed: error {}, code {} ", sdkError, exitCode));
    }

    /*
    *  Implementation of DeBot
    */

    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Tip3 DeBot";
        version = "0.4.3";
        publisher = "TON Labs";
        caption = "";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, i am a Flex Tip3 DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = "";
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