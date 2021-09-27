pragma ton-solidity ^0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/Terminal/Terminal.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/AddressInput/AddressInput.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/ConfirmInput/ConfirmInput.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/UserInfo/UserInfo.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/Menu/Menu.sol";
import "https://raw.githubusercontent.com/tonlabs/DeBot-IS-consortium/main/Sdk/Sdk.sol";
import "../interfaces/Upgradable.sol";

abstract contract Pool {
    constructor(uint256 value0, address notify, address MM) public {}
    function setProlongation(bool prol) public functionID(0xb) {}
}

contract PoolDebot is Debot, Upgradable {
    address m_MM;
    address _notify;
    TvmCell _codeP;
    TvmCell _dataP;
    uint256 m_masterPubKey;
    mapping (uint256=>address) m_addr;
    
    modifier alwaysAccept {
            tvm.accept();
            _;
    }

    function getRequiredInterfaces() public view virtual override returns (uint256[] interfaces) {
        return [Terminal.ID, AddressInput.ID, ConfirmInput.ID, UserInfo.ID, Sdk.ID];
    }

    function getDebotInfo() public functionID(0xDEB) view virtual override returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon) {
        name = "Liquidity Pool";
        version = "0.1.0";
        publisher = "TON Labs";
        caption = "Liquidity Pool";
        author = "TON Labs";
        support = address(0);
        hello = "Hi, I am a Liquidity Pool DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = "";
    }

    function setCodePool (TvmCell codeP) external alwaysAccept {
        _codeP = codeP;
    }
    
    function setDataPool (TvmCell dataP) external alwaysAccept {
        _dataP = dataP;
    }

    function start() public override {
        AddressInput.get(tvm.functionId(setStock), "Enter address of MarketMaker");
    }
    
    function setStock(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        m_MM = value;
        AddressInput.get(tvm.functionId(setnotify), "Enter address of Notify");
        enterPublicKey();
    }
    
    function setnotify(address value) public {
        Terminal.print(0, format("You entered \"{}\"", value));
        _notify = value;
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
        DeployPool();
    }
       
    function DeployPool () public {
        TvmCell _contractP = tvm.buildStateInit(_codeP, _dataP);
        _contractP = tvm.insertPubkey(_contractP, m_masterPubKey);
        m_addr[m_masterPubKey] = address.makeAddrStd(0, tvm.hash(_contractP));
        Sdk.getAccountType(tvm.functionId(PClientAccType), m_addr[m_masterPubKey]);
    }
    
    function PClientAccType(int8 acc_type) public {
        if ((acc_type==-1)||(acc_type==0)) {
            ConfirmInput.get(tvm.functionId(isDeployPClient),"You don't have MM Client. Do you want to deploy it?");
        }else if (acc_type==1){
            Terminal.print(0, "Pool Client address is:");
            Terminal.print(tvm.functionId(askMenu), format("{}",m_addr[m_masterPubKey]));
        } else if (acc_type==2){
            Terminal.print(0, format("Account {} is frozen.",m_addr[m_masterPubKey]));
        }
    }
    
    function isDeployPClient(bool value) public {
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
            isDeployPClient(true);
        }else if (acc_type==0) {
            Sdk.getBalance(tvm.functionId(checkPClientBalance), m_addr[m_masterPubKey]);
        }else if (acc_type==1){
            Terminal.print(0, format("Terminated! Account {} is already active.",m_addr[m_masterPubKey]));
        } else if (acc_type==2){
            Terminal.print(0, format("Terminated! Account {} is frozen.",m_addr[m_masterPubKey]));
        }
    }
    
    function checkPClientBalance(uint128 nanotokens) public {
        if (nanotokens<1 ton) {
            Terminal.print(0, format("Address {} balance is {:t} tons",m_addr[m_masterPubKey],nanotokens));
            isDeployPClient(true);
        }else {
            Terminal.print(tvm.functionId(DeployPClientStep2), "Deploying..");
        }
    }
    
    function DeployPClientStep2() public {
        DeployRootStep2(true);
    }

    function DeployRootStep2(bool value) public {
        if (value)
        {
            TvmCell image = tvm.buildStateInit(_codeP, _dataP);
            image = tvm.insertPubkey(image, m_masterPubKey);
            TvmCell deployMsg = tvm.buildExtMsg({
                abiVer: 2,
                dest: m_addr[m_masterPubKey],
                callbackId: tvm.functionId(askMenu),
                onErrorId: tvm.functionId(onDeployMMFailed),
                time: 0,
                expire: 0,
                sign: true,
                pubkey: m_masterPubKey,
                stateInit: image,
                call: {Pool, m_masterPubKey, _notify, m_MM}
            });
            tvm.sendrawmsg(deployMsg, 1);
        }else
            Terminal.print(0,"Terminated!");
    }
    
    function onDeployMMFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Deploy failed. Sdk error = {}, Error code = {}", sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(DeployRootStep2), "Do you want to retry?");
    }
        
    function askMenu() public {
        MenuItem[] mi;
        mi.push(MenuItem("Add stake to MarketMaker","",tvm.functionId(menustake)));
        mi.push(MenuItem("Set prolongation status","",tvm.functionId(menuprolongation)));
        mi.push(MenuItem("Show stake status","",tvm.functionId(getstakeinfo)));
        mi.push(MenuItem("Close debot","",tvm.functionId(finish)));
        Menu.select("Action list:","",mi);
    }

    function menustake() public {
        Terminal.print(0, format("Please, return after connection with AMM"));
        askMenu();
    }
    
    function getstakeinfo() public {
        Terminal.print(0, format("Please, return after connection with AMM"));
        askMenu();
    }
    
    function menuprolongation() public {
        MenuItem[] mi;
        mi.push(MenuItem("Turn on prolongation","",tvm.functionId(prolongate)));
        mi.push(MenuItem("Turn off prolongation","",tvm.functionId(noprolongate)));
        Menu.select("Action list:","",mi);
    }
    
    function prolongate() public {
       optional(uint256) none;
        Pool(m_addr[m_masterPubKey]).setProlongation{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(askMenu),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none
        }(true);
    }
    
    function noprolongate() public {
        bool now = false;
       optional(uint256) none;
        Pool(m_addr[m_masterPubKey]).setProlongation{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(askMenu),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none
        }(false);
    }
    
    function finish() public {
    
        Terminal.print(0, format("Success!"));
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

