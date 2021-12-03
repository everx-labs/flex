# FlexDebot

FlexDebot is the root trading DeBot in Flex. It invokes other DeBots as necessary.

View source code [here](https://github.com/tonlabs/flex/tree/main/debots/flex/flexDebot).

| Method                             | Invokes                                            | Description                                                        |   |
| ---------------------------------- | -------------------------------------------------- | ------------------------------------------------------------------ | - |
| setIcon                            |                                                    | Sets DeBot icon to be displayed in browsers                        |   |
| setFlexAddr                        |                                                    | Sets Flex Root address                                             |   |
| setFlexHelperDebot                 |                                                    | Sets Flex Helper DeBot Address                                     |   |
| setTip3Tip3FlexDebot               |                                                    | Sets Tip3Tip3Flex DeBot address                                    |   |
| setTip3TonFlexDebot                |                                                    | Sets Tip3Tip3Flex DeBot address (deprecated)                       |   |
| start                              | Flex Helper DeBot                                  | Starts DeBot, gets Flex Client address and keys via Flex Helper    |   |
| onGetFCAddressAndKeys              |                                                    | Checks if invoked with instant trade or opens **Main menu**        |   |
| getInstantTPCodeHash               |                                                    | Returns instant trade Trading Pair Code hash                       |   |
| setInstantTPTip3Root               |                                                    | Sets instant TON/TIP3 trade TIP3 wrapper address (deprecated)      |   |
| setInstantTPTip3MajorRoot          |                                                    | Sets instant trade TIP3 Major Root                                 |   |
| setInstantTPTip3MinorRoot          |                                                    | Sets instant trade TIP3 Minor Root                                 |   |
| startMenu                          |                                                    | Opens **Start Menu**                                               |   |
| menuTradeTip3ForTon                |                                                    | Opens TON/TIP3 Trading Menu (deprecated)                           |   |
| menuTradeTip3ForTip                |                                                    | Opens **TIP3/TIP3 Trading Menu**, initiates trading pairs search   |   |
| updateTradingPairs                 |                                                    | Searches for TIP3/TIP3 trading pairs by hash\_code                 |   |
| onAccountsByHash                   |                                                    | Iterates trading pairs search                                      |   |
| getTradingPairStock                |                                                    | Receives info on trading pair's token roots                        |   |
| setTip3MajorRoot                   |                                                    | Sets trading pair's TIP3 major root                                |   |
| getTip3MajorRootCodeHash           |                                                    | Checks that TIP3 Major root code hash belongs to Flex              |   |
| getTip3MajorRootSymbol             |                                                    | Receives TIP3 Major root contract details                          |   |
| setTip3MajorRootSymbol             |                                                    | Sets TIP3 Major root symbol (token name)                           |   |
| getTradingPairTip3MinorRoot        |                                                    | Receives info on trading pair's Minor root                         |   |
| setTip3MinorRoot                   |                                                    | Sets trading pair's TIP3 minor root                                |   |
| getTip3MinorRootCodeHash           |                                                    | Checks that TIP3 Minor root code hash belongs to Flex              |   |
| getTip3MinorRootSymbol             |                                                    | Receives TIP3 Minor root contract details                          |   |
| setTip3MinorRootSymbol             |                                                    | Sets TIP3 Minor root symbol (token name)                           |   |
| addAndNextTradingPair              |                                                    | Prints trading pair info                                           |   |
| getNextTP                          |                                                    | Continues to next trading pair                                     |   |
| showTip3Menu                       |                                                    | Displays **TIP3 Menu**                                             |   |
| menuWithdrawTons                   |                                                    | Initiates EVERs withdrawal                                         |   |
| menuUpdateTradigPair               |                                                    | Initiates Trading Pair list update                                 |   |
| onSelectSymbolMenu                 |                                                    | Looks for and/or deplots TIP3 wallet                               |   |
| getTPMinAmount                     |                                                    | Sets Trading Pair minimum trading amount                           |   |
| getTPMinAmount                     |                                                    | Sets Trading pair notification address                             |   |
| onSetTradingPairNotifyAddress      | Flex Helper DeBot                                  | Gets TIP3 Major wallet address via Flex Helper                     |   |
| onGetTip3WalletAddress             | <p>Flex Helper DeBot</p><p>Flex Tip3Tip3 DeBot</p> | Gets TIP3 Minor wallet address via Flex Helper and initiates trade |   |
| burnTip3Wallet                     |                                                    | Initiates TIP3 withdrawal from Flex wallet                         |   |
| getBurnTip3WalletFlexClientBalance |                                                    | Checks and prints TIP3 wallet balance                              |   |
| getT3WalletDetails                 |                                                    | Checks TIP3 wallet status                                          |   |
| getExtTip3WalletPublicKey          |                                                    | Checks external TIP3 Wallet pubkey                                 |   |
| getExtTip3WalletOwner              |                                                    | Requests withdrawal confirmation                                   |   |
| confirmBurn                        |                                                    | Confirms withdrawal                                                |   |
| onBurnSuccess                      |                                                    | Prints transaction success message                                 |   |
| onBurnError                        |                                                    | Prints errorr                                                      |   |
| onBurnTip3Wallet                   |                                                    | re-initiates trading pair search                                   |   |
| invokeWithBuyOrder                 | Flex Helper DeBot                                  | Debot is invoked with instant buy deal                             |   |
| invokeWithSellOrder                | Flex Helper DeBot                                  | Debot invoked with instant sell deal                               |   |
