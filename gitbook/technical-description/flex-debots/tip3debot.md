# Tip3Debot

Tip3Debot Handles TIP3 Token listing and distribution on Flex.

View source code [here](https://github.com/tonlabs/flex/tree/main/debots/flex/tip3Debot).

| Method                      | Invokes | Description                                                                                  |
| --------------------------- | ------- | -------------------------------------------------------------------------------------------- |
| setT3RCode                  |         | Set TIP3 root (wrapper) contract code                                                        |
| setT3WalletCode             |         | Set TIP3 wallet code                                                                         |
| start                       |         | Starts DeBot. Displays **Start menu**                                                        |
| menuDeployRoot              |         | Displays **Deploy root menu**, asks for user pubkey                                          |
| inputPublicKey              |         | Checks pubkey for validity, initiates root deployment procedure                              |
| menuChooseToken             |         | Displays **Choose token menu,** warns that building  token list will take time               |
| startSearching              |         | Begins searching for TIP3 root (wrapper) contracts by code hash                              |
| onDataResult                |         | Completes searching for TIP3 root (wrapper) contracts by code hash                           |
| printTokenList              |         | Prints **Token list** and **** asks to select a token or enter a TIP3 root (wrapper) address |
| menuEnterRootAddress        |         | Asks for TIP3 root (wrapper) address                                                         |
| setName                     |         | Sets selected TIP3 name                                                                      |
| setSymbol                   |         | Sets selected TIP3 symbol (short name)                                                       |
| DeployRootStep1             |         | Sets seed, calculate TIP3 root address, check account status                                 |
| DeployRootStep2             |         | Asks for TIP3 name or display unsuitable status                                              |
| DeployRootStep3             |         | Asks for TIP3 symbol                                                                         |
| DeployRootStep4             |         | Asks for TIP3 decimals                                                                       |
| DeployRootStep5             |         | Asks for TIP3 total supply value                                                             |
| DeployRootStep6             |         | Sets TIP3 total supply                                                                       |
| DeployRootStep7             |         | Asks to send tokens to sponsor deployment                                                    |
| DeployRootStep8             |         | Rechecks account status                                                                      |
| DeployRootStep9             |         | Checks account balance or display unsuitable status                                          |
| DeployRootStep9\_getBalance |         | Initializes deployment or return to DeployRootStep7, if balance is insufficient              |
| DeployRootStep10Proxy       |         | Confirms deployment                                                                          |
| DeployRootStep10            |         | Generates and sends deployment message                                                       |
| onSuccessDeployed           |         | Confirms success                                                                             |
| onDeployFailed              |         | Displays error                                                                               |
| DeployRootStep11            |         | Sets TIP3 Wallet code in TIP3 root                                                           |
| onT3WCSuccess               |         | Confirms success                                                                             |
| onT3WCError                 |         | Displays error                                                                               |
| calcTipWallet               |         | Calculate wallet address                                                                     |
| menuSetTipRoot              |         | Sets TIP3 root                                                                               |
| setTipRoot                  |         | Sets TIP3 root                                                                               |
| checkRootExistance          |         | Checks TIP3 root status                                                                      |
| checkRootCodeHash           |         | Checks TIP3 root for validity by code hash and print token details                           |
| setRootBalance              |         | Sets TIP3 root balance                                                                       |
| setTotalSupply              |         | Sets TIP3 root total supply                                                                  |
| setTotalGranted             |         | Sets TIP3 root total granted amount                                                          |
| setDecimals                 |         | Sets TIP3 root decimals                                                                      |
| setTokenName                |         | Sets TIP3 root token name                                                                    |
| setWalletCodeHash           |         | Sets Wallet Code hash  from TIP3 root                                                        |
| getTokenInfo                |         | Get token info from TIP3 root                                                                |
| printTokenDetails           |         | Print token details received from TIP3 root                                                  |
| menuGrantTokens             |         | Displays **Grant Tokens** menu                                                               |
| menuCalcAddress             |         | Asks for TIP3 wallet pubkey and calculates address from it                                   |
| menuEnterAddress            |         | Asks for TIP3 wallet address                                                                 |
| setAddress                  |         | Sets TIP3 wallet address received from user                                                  |
| setTip3WalletAddress        |         | Prints TIP3 wallet address, checks and prints wallet balance                                 |
| setGrantAmount              |         | Sets amount of TIP3 tokens to be granted and asks for confirmation                           |
| checkWallet                 |         | Checks that TIP3 wallet exists                                                               |
| startGrant                  |         | Asks for amount of TIP3 tokens to be granted                                                 |
| checkWalletBalance          |         | Checks wallet balance                                                                        |
| checkWalletStatus           |         | Checks wallet status                                                                         |
| checkWalletCodeHash         |         | Checks TIP3 wallet for validity by code hash and print token details                         |
| printWalletBalance          |         | Prints wallet balance                                                                        |
| checkWalletExistance        |         | Checks wallet status and deploys if needed                                                   |
| onDeployWalletSuccess       |         | Confirms deployment and grant success                                                        |
| onGrantSuccess              |         | Confirms grant success                                                                       |
| onDeployWalletError         |         | Displays deployment error                                                                    |
| onGrantError                |         | Displays grant error                                                                         |
