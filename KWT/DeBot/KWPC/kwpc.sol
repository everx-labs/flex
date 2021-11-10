pragma ton-solidity >=0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../abstract/Transferable.sol";
import "../library/KWPCUtils.sol";
import "../abstract/AMSig.sol";
import "../abstract/AUserToken.sol";
import "../abstract/ARootClient.sol";
import "../abstract/ADeployUserToken.sol";


contract KWPCDebot is Debot, Upgradable, Transferable {

    uint128 constant DEPLOY_VALUE = 5 ton;
    uint128 constant STAKE_MIN_VALUE = 1000 ton;
    address constant ROOT_TOKEN_ADDRESS = address(0x68618890c447cc6be870142c062f5e3f64d42c7925531615cf77e3598edd35e2);

    TvmCell m_userTokenCode;
    address m_deployUserTokenAddr;
    TvmCell m_sendMsg;
    address m_userMsig;
    uint256 m_userPubKey;
    uint32 m_userSignBox;
    address m_userTokenAddr;
    uint128 m_stakeAmount;
    uint128 m_stakePeriod;
    uint128 m_userBalance;
    uint128 m_dealAmount;
    uint128 m_withdrawTons;

    uint128 m_stake4withdraw;
    uint128 m_prevStake4withdraw;
    bool m_bCheckAirDrop;
    Order[] m_stakeOrder;
    string m_icon;

    function setIcon(string icon) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_icon = icon;
    }

    function setUserTokenCode(TvmCell code) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_userTokenCode = code;
    }

    function setDeployUserTokenAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_deployUserTokenAddr = addr;
    }

    function start() public override {
        UserInfo.getAccount(tvm.functionId(getUserAccount));
    }

    function getUserAccount(address value) public {
        if (value != address(0)) {
            m_userMsig = value;
            Sdk.getAccountType(tvm.functionId(getUserAccType), m_userMsig);
        } else {
            Terminal.print(0,"Error: User account can't be empty!");
        }

    }

    function getUserAccType(int8 acc_type) public {
        if (acc_type == 1) {
            UserInfo.getPublicKey(tvm.functionId(getUserPubKey));
        } else {
            Terminal.print(0, "Error: Your account is not active");
        }
    }

    function getUserPubKey(uint256 value) public {
        if (value != 0x0) {
            m_userPubKey = value;
            UserInfo.getSigningBox(tvm.functionId(getUserSignBox));
        } else {
            Terminal.print(0,"Error: User public key can't be zero!");
        }
    }

    function getUserSignBox(uint32 handle) public {
        m_userSignBox = handle;
        m_userTokenAddr = getUserTokenAddress();
        //Terminal.print(0,format("[DEBUG] m_userTokenAddr {}",m_userTokenAddr));
        Sdk.getAccountType(tvm.functionId(getUserTokenAccType),m_userTokenAddr);
    }

    function getUserTokenAccType(int8 acc_type) public {
        if ((acc_type == -1) || (acc_type == 0)) {
            m_userTokenAddr = address(0);
        } else if (acc_type == 2) {
            Terminal.print(0, "Error: Your UserToken account is frozen");
        }
        this.timePrint();
    }

    function getUserTokenAddress() internal view returns(address) {
        return address(tvm.hash(tvm.insertPubkey(m_userTokenCode, m_userPubKey)));
    }

    function deployUserTokenMenu() public {
        Menu.select("To participate in farming and airdrop you need account.", "", [
            MenuItem("Participate","",tvm.functionId(deployUserTokenConfirm))
        ]);
    }

    function deployUserTokenConfirm() public {
        Menu.select("Deployment will cost 5 Tons (including fee 0.1 Tons)", "", [
            MenuItem("Deploy","",tvm.functionId(deployUserTokenConfirmed)),
            MenuItem("Cancel","",tvm.functionId(timePrint))
        ]);
    }

    function deployUserTokenConfirmed() public {
        TvmCell payload = tvm.encodeBody(ADeployUserToken.deployUserToken, m_userPubKey, ROOT_TOKEN_ADDRESS);
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userMsig,
            callbackId: tvm.functionId(deployUserTokenSuccess),
            onErrorId: tvm.functionId(deployUserTokenError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AMSig.submitTransaction, m_deployUserTokenAddr, DEPLOY_VALUE, true, false, payload}
        });
        this.confirmDeployUserToken(true);
    }

    function confirmDeployUserToken(bool value) public {
        if (value) {
            Sdk.getBalance(tvm.functionId(checkMsigBalanceAmount), m_userMsig);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function checkMsigBalanceAmount(uint128 nanotokens) public {
        uint128 fee = 0.1 ton;
        if (nanotokens > DEPLOY_VALUE+fee) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Error: Your don't have enougth money on your multisig for operation! Terminated!");
        }
    }

    function deployUserTokenError(uint32 sdkError, uint32 exitCode) public {
        ConfirmInput.get(tvm.functionId(confirmDeployUserToken), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
    }

    function deployUserTokenSuccess(uint64 transId) public {
        transId;//disable compile warning
        m_userTokenAddr = getUserTokenAddress();
        timePrint();
    }


    function timePrint() public {
        if (m_userTokenAddr == address(0)) {
            mainMenu();
        } else {
            optional(uint256) none;
            AUserToken(m_userTokenAddr).getStake{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(getUserTokenStake),
                onErrorId: tvm.functionId(getMethodError),
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }
    }

    function getMethodError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Get method error. Sdk error = {}, Error code = {}", sdkError, exitCode));
        Terminal.print(tvm.functionId(this.start), "Restarting..");
    }

    function getUserTokenStake(Order[] value0) public {
        if (value0.length == 0)
            Terminal.print(0, "You have no stakes");
        else{
            m_stakeOrder = value0;
            Terminal.print(0, "Stakes:");
            for (uint128 i = 0; i < value0.length; i++){
                string str = KWCPUtils.unlockTime(value0[i].timestamp);
                Terminal.print(0, format("{} TON {} KWT {}", KWCPUtils.nanoton2str(value0[i].value), KWCPUtils.kwt2str(value0[i].returnToken), str));
            }
        }
        optional(uint256) none;
        AUserToken(m_userTokenAddr).getBalance{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getUserTokenBalance),
            onErrorId: tvm.functionId(getMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getUserTokenBalance(uint128 value0, uint128 value1, uint128 value2, uint128 value3) public {
        value0;value2;value3;
        m_withdrawTons = value0;
        if (m_withdrawTons!=0) {
            if (value2>0) {
                if (m_withdrawTons>value2) {
                    m_withdrawTons = m_withdrawTons - value2;
                }
                else  {m_withdrawTons = 0;}
            }
            if (m_withdrawTons>0.01 ton) {
                m_withdrawTons -= 0.01 ton;
            }else {
                m_withdrawTons = 0;
            }
        }

        m_stake4withdraw = value1;
        if (m_bCheckAirDrop) {
            Terminal.print(0, format("You receive {} KWT by airDrop",KWCPUtils.kwt2str(m_stake4withdraw-m_prevStake4withdraw)));
            m_prevStake4withdraw = 0;
            m_bCheckAirDrop = false;
        }
        Terminal.print(0,format("{} KWT are ready to withdraw", KWCPUtils.kwt2str(value1)));
        Sdk.getBalance(tvm.functionId(getUserTokenNativeBalance),m_userTokenAddr);
    }

    function getUserTokenNativeBalance(uint128 nanotokens) public {
        if (nanotokens<1 ton) {
            Terminal.print(0,format("Balance of your contract is low. You need to replenish your user contract.\nUser contract address {}\n Balance {:t}",m_userTokenAddr,nanotokens));
        }
        if (nanotokens<0.5 ton) {
            Menu.select("You need to replenish your user contract to continue participating.", "", [
                MenuItem("Replenish","",tvm.functionId(replenishUserContract))
            ]);
        } else {
            this.mainMenu();
        }
    }

    function replenishUserContract() public {
        Sdk.getBalance(tvm.functionId(getUserMsigReplenishBalance),m_userMsig);
    }

    function getUserMsigReplenishBalance(uint128 nanotokens) public{
        AmountInput.get(tvm.functionId(replenishUserContractAmount),"How many Tons do you want to tranfer?",9,0.2 ton,nanotokens - 0.1 ton);
    }

    function replenishUserContractAmount(uint128 value) public {
        m_dealAmount = value;
        TvmCell payload;
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userMsig,
            callbackId: tvm.functionId(replenishUserContractSuccess),
            onErrorId: tvm.functionId(replenishUserContractError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AMSig.submitTransaction, m_userTokenAddr, m_dealAmount, true, false, payload}
        });
        this.confirmReplenishUserToken(true);
    }

    function confirmReplenishUserToken(bool value) public {
        if (value) {
            Sdk.getBalance(tvm.functionId(checkReplenisMsigBalanceAmount), m_userMsig);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function checkReplenisMsigBalanceAmount(uint128 nanotokens) public {
        uint128 fee = 0.1 ton;
        if (nanotokens > m_dealAmount+fee) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Error: Your don't have enougth money on your multisig for operation! Terminated!");
        }
    }

    function replenishUserContractError(uint32 sdkError, uint32 exitCode) public {
        ConfirmInput.get(tvm.functionId(confirmReplenishUserToken), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
    }

    function replenishUserContractSuccess(uint64 transId) public {
        transId;//disable compile warning
        timePrint();
    }

    function mainMenu() public {
        int32 difTime = (int32(now) - int32(KWCPUtils.PODCAST_START));
        if (difTime<0)
        {
            uint32 time = uint32(math.abs(difTime));
            (uint8 day, uint8 hour, uint8 min, uint8 sec) = KWCPUtils.timeBefore(time);
            string str = "Time until event start: ";
            if (day == 1) {str.append(format("{} DAY ",day));} else {str.append(format("{} DAYS ",day));}
            if (hour == 1) {str.append(format("{} HOUR ",hour));} else {str.append(format("{} HOURS ",hour));}
            if (min == 1) {str.append(format("{} MINUTE ",min));} else {str.append(format("{} MINUTES ",min));}
            if (sec == 1) {str.append(format("{} SECOND",sec));} else {str.append(format("{} SECONDS",sec));}
            Terminal.print(0, str);
            if (m_userTokenAddr == address(0)) {
                this.deployUserTokenMenu();
            }else{
                Menu.select("Stay in touch!", "", [
                    MenuItem("Ok","",tvm.functionId(timePrint))
                ]);
            }

        } else if (difTime<3600) {
            uint32 time = uint32(math.abs(difTime));
            (uint8 min, uint8 sec) = KWCPUtils.timeAfter(time);
            string str = "WhaleDrop is running for ";
            if (min == 1) {str.append(format("{} MINUTE ",min));} else {str.append(format("{} MINUTES ",min));}
            if (sec == 1) {str.append(format("{} SECOND",sec));} else {str.append(format("{} SECONDS",sec));}
            Terminal.print(0, format("This minute stake price is {} Tons. This minute total stake is {} Tons",KWCPUtils.getKWTStrPrice(min), KWCPUtils.getTotalSupplyAtMinute(min)));
            if (min<59) {
               Terminal.print(0, format("Next minute stake price is {} Tons. Next minute total stake is {} Tons",KWCPUtils.getKWTStrPrice(min+1), KWCPUtils.getTotalSupplyAtMinute(min+1)));
            } else {
               Terminal.print(0, "It is last chance to stake");
            }
            if (m_userTokenAddr == address(0)) {
                this.deployUserTokenMenu();
            }else{
                Menu.select(str, "", [
                    MenuItem("Stake and airdrop","",tvm.functionId(stake)),
                    MenuItem("Airdrop only","",tvm.functionId(airDrop)),
                    MenuItem("Update","",tvm.functionId(timePrint))
                ]);
            }
        } else {
            MenuItem[] items;
            if(m_withdrawTons>0) {
                items.push(MenuItem("Withdraw tons","",tvm.functionId(withdrawTons)));
            }
            items.push(MenuItem("Ok","",tvm.functionId(timePrint)));

            Terminal.print(0, "WhaleDrop is over!");
            Menu.select("Stay in touch!", "", items);
        }
    }

/*
* withdraw tons
*/
    function withdrawTons() public {
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userTokenAddr,
            callbackId: tvm.functionId(withdrawTonsSuccess),
            onErrorId: tvm.functionId(withdrawTonsError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AUserToken.sendMoney,m_withdrawTons,m_userMsig}
        });
        ConfirmInput.get(tvm.functionId(confirmWithdrawTons), format("Do you want to transfer {:t} TONs to your accont {}?", m_withdrawTons, m_userMsig));
    }

    function confirmWithdrawTons(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function withdrawTonsError(uint32 sdkError, uint32 exitCode) public {
        ConfirmInput.get(tvm.functionId(confirmWithdrawTons), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
    }

    function withdrawTonsSuccess() public {
        timePrint();
    }

/*
* air drop
*/
    function airDrop() public {
        m_bCheckAirDrop = true;
        m_prevStake4withdraw = m_stake4withdraw;
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userTokenAddr,
            callbackId: tvm.functionId(airDropSuccess),
            onErrorId: tvm.functionId(airDropError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AUserToken.getAirdrop}
        });
        confirmAirDrop(true);
    }

    function confirmAirDrop(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function airDropError(uint32 sdkError, uint32 exitCode) public {
        if ((sdkError==414)&&(exitCode==505)) {
            Terminal.print(tvm.functionId(timePrint),"Time of WhaleDrop is over!");
        } else {
            ConfirmInput.get(tvm.functionId(confirmAirDrop), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
        }
    }

    function airDropSuccess() public {
        timePrint();
    }
/*
* update stakes
*/

    function updateStakes() public {
        m_prevStake4withdraw = m_stake4withdraw;
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userTokenAddr,
            callbackId: tvm.functionId(updateStakesSuccess),
            onErrorId: tvm.functionId(updateStakesError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AUserToken.update}
        });
        updateStakesDrop(true);
    }

    function updateStakesDrop(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function updateStakesError(uint32 sdkError, uint32 exitCode) public {
        ConfirmInput.get(tvm.functionId(updateStakesDrop), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
    }

    function updateStakesSuccess() public {
        timePrint();
    }

/*
* stake
*/
    function stake() public {
        Sdk.getBalance(tvm.functionId(checkStakeMsigBalance), m_userMsig);
    }

    function checkStakeMsigBalance(uint128 nanotokens) public {
        uint128 fee = 0.6 ton;
        if (nanotokens > STAKE_MIN_VALUE+fee) {
            AmountInput.get(tvm.functionId(getStakeAmount), "Amount of tons you want to stake (0.5 ton will be added as fee):",9,STAKE_MIN_VALUE,nanotokens-fee);
        } else {
            Terminal.print(tvm.functionId(this.start), "Error: Minimal stake is 1000. Your don't have enougth money on your multisig for operation! Terminated!");
        }
    }

    function getStakeAmount(uint128 value) public {
        m_stakeAmount = value;
        Menu.select("Select lock period", "", [
            MenuItem("90 days - 15%","",tvm.functionId(getLockPeriod)),
            MenuItem("180 days - 35%","",tvm.functionId(getLockPeriod)),
            MenuItem("360 days - 75%","",tvm.functionId(getLockPeriod))
        ]);
    }

    function getLockPeriod(uint32 index) public {
        if (index==0) m_stakePeriod = 7776000;
        else if (index==1) m_stakePeriod = 15552000;
        else if (index==2) m_stakePeriod = 31104000;
        runStake();
    }

    function runStake() public {
        TvmCell payload = tvm.encodeBody(ARootClient.addStake, m_stakeAmount, m_stakePeriod, m_userTokenAddr);
        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_userMsig,
            callbackId: tvm.functionId(stakeSuccess),
            onErrorId: tvm.functionId(stakeError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            signBoxHandle: m_userSignBox,
            call: {AMSig.submitTransaction, ROOT_TOKEN_ADDRESS, m_stakeAmount + 0.5 ton, true, false, payload}
        });

        confirmStake(true);
    }

    function confirmStake(bool value) public {
        if (value) {
            tvm.sendrawmsg(m_sendMsg, 1);
        } else {
            Terminal.print(tvm.functionId(this.start), "Terminated!");
        }
    }

    function stakeError(uint32 sdkError, uint32 exitCode) public {
        ConfirmInput.get(tvm.functionId(confirmStake), format("Transaction failed. Sdk error = {}, Error code = {}\nDo you want to retry?", sdkError, exitCode));
    }

    function stakeSuccess(uint64 transId) public {
        transId;
        ConfirmInput.get(tvm.functionId(confirmReceiveAirDrop), "Do you want to receive air drop?");
    }

    function confirmReceiveAirDrop(bool value) public {
        if (value) {
            airDrop();
        } else {
            timePrint();
        }
    }
    /*
    *  Implementation of DeBot
    */

    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "KWPC";
        version = "0.1.1";
        publisher = "TON Labs";
        key = "";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, I'm Killer Whale Pod Cast DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = m_icon;
    }

    function getRequiredInterfaces() public view override returns (uint256[] interfaces) {
        return [ Terminal.ID, Menu.ID, UserInfo.ID, Sdk.ID, ConfirmInput.ID, AmountInput.ID ];
    }
    /*
    *  Implementation of Upgradable
    */
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}