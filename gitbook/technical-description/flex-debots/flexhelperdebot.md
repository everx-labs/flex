# FlexHelperDebot

FlexHelperDebot works with user Flex Client and Flex TIP3 wallet contracts when invoked by FlexDebot.

View source code [here](https://github.com/tonlabs/flex/tree/main/debots/flex/flexHelperDebot).

| Method                       | Invokes   | Description                                                                  |
| ---------------------------- | --------- | ---------------------------------------------------------------------------- |
| setFlexClientCode            |           | Sets Flex Client code                                                        |
| setFlexWalletCode            |           | Sets Flex Wallet code                                                        |
| start                        |           | Starts debot. Prints error message, if accidentally invoked by user directly |
| getFCAddressAndKeys          |           | Gets Flex Client address and keys                                            |
| getPublicKey                 |           | Gets user public key                                                         |
| getSigningBox                |           | Accesses signing box data                                                    |
| getTradingPairCode           |           | Gets trading pair code from Flex root                                        |
| setTradingPairCode           |           | Sets trading pair root                                                       |
| setXchgPairCode              |           | Sets trading pair root                                                       |
| getFlexTonsCfg               |           | Gets flex configuration                                                      |
| setFlexTonsCfg               |           | Sets flex configuration                                                      |
| getFlexDealsLimit            |           | gets flex deals limit                                                        |
| FlexClientAccType            |           | Checks and prints Flex Client status, offers to deploy if missing            |
| deployFlexClient             |           | Initiates Flex Client deploy                                                 |
| clientAccType                |           | Checks user account status                                                   |
| getClientBalance             |           | Asks for desired Flex Client balance                                         |
| getBalanceToSend             |           | Gets amount of tokens to send to Flex Client                                 |
| grantFlexClient              |           | Sends tokens to Flex Client balance                                          |
| onFCGrantSuccess             |           | Confirms success                                                             |
| onFCGrantError               |           | Displays error                                                               |
| deployFlexClientCode         |           | Deploys Flex Client code                                                     |
| onSuccessFCDeployed          |           | Confirms success                                                             |
| onDeployFCFailed             |           | Displays error                                                               |
| flexClientSetStockCfg        |           | Configures flex client                                                       |
| onSuccessFCSetTonsCfg        |           | Confirms success                                                             |
| onGetFCWalletCode            |           | Checks Wallet code in Flex client status                                     |
| hasFlexWalletCode            |           | Checks Wallet code in Flex client status                                     |
| flexClientSetWalletCode      |           | Sets Wallet code in Flex client                                              |
| onSuccessFCSetFlexWalletCode |           | Confirms success                                                             |
| onFCSetFlexWalletCodeError   |           | Displays error                                                               |
| onFCSetTonsCfgError          |           | Displays error                                                               |
| checkFlexClientStockCfg      |           | Checks Flex Client configuration                                             |
| getFlexFlex                  |           | Checks Flex Client configuration                                             |
| onGetFCAddressAndKeys        | FlexDebot | Sends Flex Client info to FlexDebot                                          |
| getTip3WalletAddress         |           | Gets TIP3 Wallet address for specific TIP3 token                             |
| setTip3WrapperSymbol         |           | Sets TIP3 token name                                                         |
| setTip3WalletAddress         |           | Sets TIP3 Wallet address                                                     |
| Tip3WalletAccType            |           | Check user TIP3 Wallet status                                                |
| enterMsigAddr                |           | Requires multisig wallet address to sponsor deployment                       |
| getDeplotT3WClientAccType    |           | Checks provided multisig wallet status                                       |
| getDeplotT3WClientBalance    |           | Asks token amount to sponsor deployment                                      |
| enterMsigTons                |           | Gets token amount                                                            |
| deployEmptyWallet            |           | Deploys empty Flex TIP3 Wallet                                               |
| onDEWSuccess                 |           | Confirms success                                                             |
| onDEWError                   |           | Displays error                                                               |
| checkT3WWait                 |           | Check TIP3 Wallet status                                                     |
| Tip3WalletAccTypeWait        |           | Check TIP3 Wallet status                                                     |
| onGetTip3WalletAddress       | FlexDebot | Sends TIP3 Wallet address to FlexDebot                                       |
| withdrawTons                 |           | Initiates tokens withdrawal from Flex Client                                 |
| getFlexClientBalance         |           | Gets Flex Client Balance and asks for amount to withdraw                     |
| enterWithdrawalTons          |           | Asks for address to which to withdraw                                        |
| enterWithdrawalAddr          |           | Generates message and asks for withdrawal confirmation                       |
| confirmWithdrawTons          |           | Receives confirmation and sends message                                      |
| onWithdrawSuccess            |           | Confirms success                                                             |
| onWithdrawError              |           | Displays error                                                               |
| onWithdrawTons               | FlexDebot | Invokes Trading pair list update in FlexDebot                                |

