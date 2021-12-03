# FlexTip3Tip3Debot

FlexTip3Tip3Debot performs TIP3/TIP3 trades when invoked by FlexDebot.&#x20;

View source code [here](https://github.com/tonlabs/flex/tree/main/debots/flex/flexTip3Tip3Debot).

| Method                     | Invokes   | Description                                                             |
| -------------------------- | --------- | ----------------------------------------------------------------------- |
| start                      |           | Starts DeBot                                                            |
| tokensToStr                |           | Converts token amount to string                                         |
| instantTradeTip3Tip3       |           | Performs instant trade                                                  |
| setInstantPriceCode        |           | Sets instant trade price contract code                                  |
| tradeTip3Tip3              |           | Performs trade                                                          |
| getFCT3WBalances           |           | Checks Flex Client balance                                              |
| getT3WDetails              |           | Checks TIP3 major wallet balance                                        |
| getT3WMajorDetails         |           | Checks TIP3 minor wallet balance                                        |
| setT3WDetails              |           | Sets and prints both TIP3 Wallet details, gets order book               |
| setPriceCode               |           | Sets Price contract code                                                |
| setUpdateT3WDetails        |           | Update TIP3 wallet details                                              |
| getPricesDataByHash        |           | Reads Price contracts data by hash code                                 |
| onPricesByHash             |           | Displays **TIP 3 wallet details and order book menu**                   |
| menuCancelBuyTip3          |           | Displays **Cancel buy menu**                                            |
| menuCancelSellTip3         |           | Displays **Cancel sell menu**                                           |
| showCancelOrderMenu        |           | Displays **Cancel order menu**                                          |
| menuCancelSellTip3Order    |           | Displays **Cancel sell order menu**                                     |
| cancelSellOrder            |           | Generates Cancel sell order message and asks for confirmation           |
| confirmCancelSellOrder     |           | Receives confirmation or initiates order book update                    |
| onSendCancelSellOrder      |           | Sends Cancel sell order message                                         |
| onSellOrderCancelSuccess   |           | Confirms success                                                        |
| onSellOrderCancelError     |           | Displays error                                                          |
| menuQuickDeal              |           | Displays **Fast trade** **menu**                                        |
| getBuyQuickDealAmount      |           | Gets token amount for fast buy                                          |
| getSellQuickDealAmount     |           | Gets token amount for fast sell                                         |
| menuOrderBookUpdate        |           | Initiates **TIP 3 wallet details and order book menu** update           |
| menuOrderBookBack          | FlexDebot | Returns user to FlexDebot and initiates traiding pair list update       |
| menuOrderBookBurnTip3      |           | Displays **Withdraw TIP3 menu**                                         |
| menuOrderBookBurnMajor     | FlexDebot | Returns user to FlexDebot and initiates Major wallet withdrawal         |
| menuOrderBookBurnMinor     | FlexDebot | Returns user to FlexDebot and initiates Major wallet withdrawal         |
| menuBuyTip3                |           | Ask how many TIP3 to buy                                                |
| getDealAmmount             |           | Ask for price of trade                                                  |
| getDealPrice               |           | Ask for order lifetime                                                  |
| getBuyDealTime             |           | Calculate order lifetime in seconds, check Flex Client contract balance |
| deployPriceWithBuy         |           | Generate deploy Price contract message and ask for confirmation         |
| confirmDeployPriceWithBuy  |           | Receive confirmation or rebuild order book                              |
| onSendDeployBuy            |           | Send deploy buy order message                                           |
| onBuySuccess               |           | Confirms success                                                        |
| onBuyError                 |           | Displays error                                                          |
| menuSellTip3               |           | Ask how many TIP3 to sell                                               |
| getSellDealAmmount         |           | Ask for price of trade                                                  |
| getSellDealPrice           |           | Ask for order lifetime                                                  |
| getSellDealTime            |           | Calculate order lifetime in seconds, check Flex Client contract balance |
| deployPriceWithSell        |           | Generate deploy Price contract message and ask for confirmation         |
| confirmDeployPriceWithSell |           | Receive confirmation or rebuild order book                              |
| onSendDeploySell           |           | Send deploy sell order message                                          |
| onSellSuccess              |           | Confirms success                                                        |
| onSellError                |           | Displays error                                                          |
