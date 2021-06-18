pragma ton-solidity >=0.35.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
import "../interfaces/Debot.sol";
import "../interfaces/Upgradable.sol";
import "../interfaces/Transferable.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/Menu.sol";
import "../interfaces/AmountInput.sol";
import "../interfaces/ConfirmInput.sol";
import "../interfaces/Msg.sol";
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

abstract contract ATip3Root {
    constructor(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint256 root_owner, uint128 total_supply) public {}
    function getName() public returns(string value0) {}
    function setWalletCode(TvmCell wallet_code) public {}
    function deployWallet(uint32 _answer_id, int8 workchain_id, uint256 pubkey, uint256 internal_owner,
        uint128 tokens, uint128 grams) public functionID(1888586564) returns(address value0) {}
    function grant(address dest, uint128 tokens, uint128 grams) public {}

    function getWalletAddress(int8 workchain_id, uint256 pubkey, uint256 owner_std_addr) public returns(address value0) {}
    function getDecimals() public returns (uint8 value0) {}
    function getTotalSupply() public returns (uint128 value0) {}
    function getTotalGranted() public returns (uint128 value0) {}
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

    string m_seedphrase;
    uint128 m_rootBalance;
    uint128 m_grantAmount;
    uint128 m_nativeAmount;

    address m_flexClient;

    TvmCell m_t3rContractCode;
    TvmCell m_t3WalletCode;
    TvmCell m_flexClientCode;

    address m_deployAddress;

    uint32 m_seedPhraseCallBack;

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

    function setFlexClientCode(TvmCell code) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexClientCode = code;
    }

    function start() public override {
        Terminal.print(0, "Hi, I can help you manage TIP3 Tokens.");
        Menu.select("Where do we start?", "", [
            MenuItem("Choose TIP3 token","",tvm.functionId(menuChooseToken)),
            MenuItem("Mint new TIP3 token","",tvm.functionId(menuDeployRoot))
        ]);
    }

    function menuDeployRoot(uint32 index) public {
        m_seedPhraseCallBack = tvm.functionId(DeployRootStep1);
        Terminal.print(0, "At this step you need to have a seed phrase that will be used to manage your TIP3 Token.\nKeep it in secret!\nDon't forget it!");
        Menu.select("Choose what to do?", "",[
            MenuItem("Generate a seed phrase for me","",tvm.functionId(menuGenSeedPhrase)),
            MenuItem("I have the seed phrase","",tvm.functionId(menuEnterSeedPhrase))
        ]);
    }

    function menuChooseToken(uint32 index) public {
        AddressInput.get(tvm.functionId(setTipRoot), "To identify TIP3 token i need to know its Root address.\nEnter TIP3 Root address:");
    }

    function menuCalcWallet(uint32 index) public {
        Terminal.print(0, "Be sure that you already have a FLEX seed phrase. Otherwise go to FLEX DeBot for help.");
        m_seedPhraseCallBack = tvm.functionId(calcFlexClient);
        menuEnterSeedPhrase(0);
    }

    function menuGenSeedPhrase(uint32 index) public {
        Sdk.mnemonicFromRandom(tvm.functionId(showMnemonic),1,12);
    }
    function showMnemonic(string phrase) public {
        Terminal.print(0, "Generated seed phrase:");
        Terminal.print(0, phrase);
        Terminal.print(0, "Important! Please, don't forget it!");
        menuEnterSeedPhrase(0);
    }

    function menuEnterSeedPhrase(uint32 index) public {
        Terminal.input(tvm.functionId(checkSeedPhrase),"Enter your seed phrase:",false);
    }
    function checkSeedPhrase(string value) public {
        m_seedphrase = value;
        Sdk.mnemonicVerify(tvm.functionId(verifySeedPhrase),m_seedphrase);
    }
    function verifySeedPhrase(bool valid) public {
        if (valid){
            getMasterKeysFromMnemonic(m_seedphrase);
        }else{
            Terminal.print(tvm.functionId(Debot.start),"Error: invalid seed phrase! Try to enter it without quotes or generate a new one.");
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
        Terminal.print(m_seedPhraseCallBack,format("Key pair:\n  public: {:x}\n  secret: {:x}",pub,sec));
    }
//////////////////////////////////////////////////////////////////////

    function DeployRootStep1() public {
        TvmCell deployState = tvm.insertPubkey(m_t3rContractCode, m_masterPubKey);
        m_deployAddress = address.makeAddrStd(0, tvm.hash(deployState));
        Sdk.getAccountType(tvm.functionId(DeployRootStep2), m_deployAddress);
    }
    function DeployRootStep2(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            Terminal.input(tvm.functionId(DeployRootStep3),"Enter tip3 name",false);
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is active. Try to use another seed phrase",m_deployAddress));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is frozen. Try to use another seed phrase",m_deployAddress));
        }
    }
    function DeployRootStep3(string value) public {
        m_name = value;
        Terminal.input(tvm.functionId(DeployRootStep4),"Enter tip3 symbol",false);
    }
    function DeployRootStep4(string value) public {
        m_symbol = value;
        AmountInput.get(tvm.functionId(DeployRootStep5),"Enter tip3 decimals",0,0,0xFF);
    }
    function DeployRootStep5(int256 value) public {
        m_decimals = uint8(value);
        AmountInput.get(tvm.functionId(DeployRootStep6),"Enter tip3 total supply",m_decimals,0,0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
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
            Terminal.print(tvm.functionId(DeployRootStep10Proxy), "Initializing your TIP3 Token Root");
        }else if (acc_type==1){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is already active.",m_deployAddress));
        } else if (acc_type==2){
            Terminal.print(tvm.functionId(Debot.start), format("Account {} is frozen.",m_deployAddress));
        }
    }
    function DeployRootStep10Proxy() public {
        DeployRootStep10(true);
    }
    function DeployRootStep10(bool value) public {
        if (value)
        {
            TvmCell image = tvm.insertPubkey(m_t3rContractCode, m_masterPubKey);
            optional(uint256) none;
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_deployAddress,
                callbackId: tvm.functionId(onSuccessDeployed),
                onErrorId: tvm.functionId(onDeployFailed),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: none,
                stateInit: image,
                call: {ATip3Root,m_name,m_symbol,m_decimals,m_masterPubKey,0x0,m_totalSupply}
            });
            Msg.sendWithKeypair(tvm.functionId(onSuccessDeployed),deployMsg,m_masterPubKey,m_masterSecKey);
        }else
            Terminal.print(tvm.functionId(Debot.start),"Terminated!");
    }

    function onSuccessDeployed() public {
        Terminal.print(0, "TIP3Token created.");
        Terminal.print(tvm.functionId(DeployRootStep11), "A few more seconds, please.\nUploading wallet code...");
    }

    function onDeployFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep10), "Do you want to retry?");
    }

    function DeployRootStep11() public {
        optional(uint256) none;
        TvmCell m_extMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_deployAddress,
            callbackId: tvm.functionId(onT3WCSuccess),
            onErrorId: tvm.functionId(onT3WCError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {ATip3Root.setWalletCode,m_t3WalletCode}
        });
        Msg.sendWithKeypair(tvm.functionId(onT3WCSuccess),m_extMsg,m_masterPubKey,m_masterSecKey);
    }

    function onT3WCSuccess() public {
        Terminal.print(0, "Congrats! Now you have your own TIP3 token. Please, remember TIP3 Root address:");
        Terminal.print(tvm.functionId(Debot.start), format("{}", m_deployAddress));
    }
    function onT3WCError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep11), "retrying..");
    }

//////////////////////////////////////////////////////////////////////
    function calcFlexClient() public {
        m_flexClient = _checkFlexClient(m_flexClientCode, m_masterPubKey);
        Sdk.getAccountType(tvm.functionId(flexClientAccType), m_flexClient);
    }

    function _checkFlexClient(TvmCell cliCode, uint256 pubKey) internal returns (address) {
        TvmCell deployState = tvm.insertPubkey(cliCode, pubKey);
        address flexClient = address.makeAddrStd(0, tvm.hash(deployState));
        return flexClient;
    }

    function flexClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            Terminal.print(tvm.functionId(Debot.start),"Your FLEX Client is inactive. Go to FLEX DeBot and create it.");
        } else if (acc_type==1) {
            Terminal.print(0, format("FLEX Client is ready.\nAddress: {}", m_flexClient));
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
            }(0, 0x0, m_flexClient.value);

        } else if (acc_type==2) {
            Terminal.print(tvm.functionId(Debot.start), format("FLEX client is frozen. Address: {}", m_flexClient));
        }
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
    }

    function printTokenDetails() public {
        Terminal.print(0, format("Name: {}\nTotal supply: {}\nTotal granted: {}",
            m_name, tokensToStr(m_totalSupply, m_decimals), tokensToStr(m_totalGranted, m_decimals)));

        Menu.select("What's next?", "", [MenuItem("Grant tokens", "", tvm.functionId(menuGrantTokens))]);
    }

    function menuGrantTokens(uint32 index) public {
        Menu.select("Giv me a recipient address:", "", [
            MenuItem("Enter TIP3 wallet address", "", tvm.functionId(menuEnterAddress)),
            MenuItem("Calculate TIP3 wallet address from FLEX seed phrase", "", tvm.functionId(menuCalcWallet))
        ]);
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
        AmountInput.get(tvm.functionId(setDeployAomunt), "How many native tokens send to this wallet?", 9, 0, m_rootBalance);
    }

    function setDeployAomunt(uint128 value) public {
        m_nativeAmount = value;
        ConfirmInput.get(
            tvm.functionId(checkWallet),
            format("Ok, i'm ready to grant {} TIP3 tokens and {} native tokens to the wallet {}.\nConfirm and sign with TIP3 root seed phrase.",
            tokensToStr(m_grantAmount, m_decimals), tokensToStr(m_nativeAmount, 9), m_tip3wallet)
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
        AmountInput.get(tvm.functionId(setGrantAmount), "How many TIP3 tokens grant to this wallet?", m_decimals, 0, m_totalSupply - m_totalGranted);
    }

    function checkWalletBalance() public {
        Sdk.getAccountType(tvm.functionId(checkWalletStatus), m_tip3wallet);
    }

    function checkWalletStatus(int8 acc_type) public {
        if (acc_type == 1) {
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
            Terminal.print(0, "TIP3 Balance: 0");
            startGrant();
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
            }(0, 0, 0, m_flexClient.value, m_grantAmount, m_nativeAmount);
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
        Terminal.print(0, "Succeeded. TIP3 wallet deployed and granted with tokens.");
    }

    function onGrantSuccess() public {
        Terminal.print(0, "Succeeded. TIP3 wallet granted with tokens.");
    }

    function onDeployWalletError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed: error {}, code {} ", sdkError, exitCode));
    }

    function onGrantError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Granting failed: error {}, code {} ", sdkError, exitCode));
    }

    /*
    *  Implementation of DeBot
    */

    function getVersion() public override returns (string name, uint24 semver) {
        (name, semver) = ("Tip3 DeBot from TON Labs", _version(0,3,0));
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