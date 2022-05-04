#!/bin/bash
set -e
filename=flexTip3Tip3Debot
filenamesol=$filename.sol
filenameabi=$filename.abi.json
filenametvc=$filename.tvc
filenamekeys=$filename.keys.json

CLI_PATH=~/git/tonos-cli/target/release
LOCAL_GIVER_PATH=~/givers/local_giver
NET_GIVER_PATH=~/givers/net_giver
#DEPLOY_LOCAL=1

#$CLI_PATH/tonos-cli config --url http://localhost
#$CLI_PATH/tonos-cli config --url https://main.ton.dev

function giver_local {
$CLI_PATH/tonos-cli call --abi $LOCAL_GIVER_PATH/giver.abi.json 0:841288ed3b55d9cdafa806807f02a0ae0c169aa5edfe88a789a6482429756a94 sendGrams "{\"dest\":\"$1\",\"amount\":5000000000}"
}
function giver_net {
$CLI_PATH/tonos-cli call 0:2bb4a0e8391e7ea8877f4825064924bd41ce110fce97e939d3323999e1efbb13 sendTransaction "{\"dest\":\"$1\",\"value\":5000000000,\"bounce\":\"false\"}" --abi $NET_GIVER_PATH/giver.abi.json --sign $NET_GIVER_PATH/keys.json
}
function get_address {
echo $(cat log.log | grep "Raw address:" | cut -d ' ' -f 3)
}

echo ""
echo "[DEBOT]"
echo ""

echo GENADDR DEBOT
$CLI_PATH/tonos-cli genaddr $filenametvc $filenameabi --genkey $filenamekeys > log.log
debot_address=$(get_address)
echo GIVER
giver_net $debot_address
echo DEPLOY DEBOT
$CLI_PATH/tonos-cli deploy $filenametvc "{}" --sign $filenamekeys --abi $filenameabi
echo SET DEBOT ABI
debot_abi=$(cat $filenameabi | jq -c '.' | xxd -ps -c 200000)
$CLI_PATH/tonos-cli call $debot_address setABIBytes "{\"dabi\":\"$debot_abi\"}" --sign $filenamekeys --abi $filenameabi

echo DONE
echo $debot_address > address.log
