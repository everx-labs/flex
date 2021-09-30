# Flex Help

# What is Flex?

Flex (stands for Flash exchange) is a decentralized exchange on Free TON, which allows users to buy and sell TIP3 distributed tokens listed on it. Tokens may be exchanged for the native TON Crystal tokens and for each other.

Flex operates as a system of open sourced smart contracts, optimized for speed and security. All buy and sell actions are performed through a specialized Flex [DeBot](https://help.ton.surf/en/support/solutions/folders/77000278262), which is supported by DeBot browsers such as [TON Surf](https://ton.surf/main). It helps the user create and manage all contracts required to trade on Flex.

Flex website is located at [https://flex.ton.surf/](https://flex.ton.surf/)

Flex, Flex DeBot and distributed token source code is available [here](https://github.com/tonlabs/flex).

# What can I trade?

You can buy any listed tokens, or sell any listed tokens you own. 

> **Note**: Flex is available both on the Free TON Mainnet, and on the Devnet, but currently Mainnet Flex only has Test tokens listed (TEST1 and TEST2), so it isn't advisable to spend a lot of Tons on them.

Always choose carefully what you wish to trade and on what network you are going to do it.

Use the drop-down network list to switch to Devnet and back:

![https://github.com/tonlabs/flex/raw/main/screenshots/help1.png](https://github.com/tonlabs/flex/raw/main/screenshots/help1.png)

Use the token drop-down list to select a token you want to trade:

![https://github.com/tonlabs/flex/raw/main/screenshots/help2.png](https://github.com/tonlabs/flex/raw/main/screenshots/help2.png)

# How to buy/sell

Flex displays all available information about a token which can help you decide on a trade. A few graphs visualize token status and history, while the **Orderbook** and **Trades** tabs list the currently available orders and accumulated trade statistics.

![https://github.com/tonlabs/flex/raw/main/screenshots/help3.png](https://github.com/tonlabs/flex/raw/main/screenshots/help3.png)

Decide on a trade, and fill in your order in the rightmost section of the page: 

![https://github.com/tonlabs/flex/raw/main/screenshots/help4.png](https://github.com/tonlabs/flex/raw/main/screenshots/help4.png)

You can set your own price and amount, or click on an order in the **Orderbook** to fulfill it.

- **When buying**: if the tokens you want to buy are available in the amount you're requesting at a price equal to the one you specified, Flex will facilitate the trade immediately for however many currently available tokens fit your conditions and create a buy order for the rest. If none are available, a buy order will be created for the whole amount.
- **When selling**: if there is a suitable buy order for the tokens you want to sell, or a part of it, the trade will be executed immediately, otherwise a buy order will be created for part or all of the tokens you are selling.

The buy and sell orders are valid until fulfilled or cancelled.

Click the **Buy...** or **Sell...** button, once you're ready. You'll be redirected to Surf to complete the trade with the help of the **Flex DeBot**.

**If this is your first time trading**, the DeBot will help you deploy the necessary contracts (your **Flex Client** contract and your **TIP3 token wallet**). 

![https://github.com/tonlabs/flex/raw/main/screenshots/help5.png](https://github.com/tonlabs/flex/raw/main/screenshots/help5.png)

You might have to confirm several transactions and pay a few small fees during this process.

Make sure to transfer enough Tons for any trades you want to perform to your Flex Client contract.

> **Note**: If you run out of funds on your Flex Client contract, you'll be able to top it up [by a simple transfer of Tons](https://help.ton.surf/en/support/solutions/articles/77000272176-how-do-i-send-coins-). Flex DeBot will tell you its address.
> 

If any other Flex contracts are missing, the DeBot will help you prepare them too.

**Once all required contracts are ready**, the DeBot will summarize the trade transaction for you and ask for a confirmation. Confirm it, if you are sure.

![https://github.com/tonlabs/flex/raw/main/screenshots/help6.png](https://github.com/tonlabs/flex/raw/main/screenshots/help6.png)

When all actions are complete, DeBot will display your Flex Client and TIP3 wallet balances, the updated state of the orderbook, and your options for further trading.

![https://github.com/tonlabs/flex/raw/main/screenshots/help7.png](https://github.com/tonlabs/flex/raw/main/screenshots/help7.png)

You can use it to set up any further trades, or return to the [flex.ton.surf](http://flex.ton.surf) website, if it is more convenient to you.

Flex DeBot also allows you to cancel orders and withdraw tokens to external wallets:

![https://github.com/tonlabs/flex/raw/main/screenshots/help8.png](https://github.com/tonlabs/flex/raw/main/screenshots/help8.png)

Flex DeBot address in Surf:
[https://uri.ton.surf/debot/0:e57e4fa3f5ab829c12ce145ce24c0a9b4aa666a0e60d5b00f8bfd6aa1d90d64f](https://uri.ton.surf/debot/0:e57e4fa3f5ab829c12ce145ce24c0a9b4aa666a0e60d5b00f8bfd6aa1d90d64f)

![https://github.com/tonlabs/flex/raw/main/screenshots/flex_qr.png](https://github.com/tonlabs/flex/raw/main/screenshots/flex_qr.png)

# Learn more

Here is some further reading about the technologies used in Flex:

Flex repository

[https://github.com/tonlabs/flex](https://github.com/tonlabs/flex)

DeBots

 [https://github.com/tonlabs/debots](https://github.com/tonlabs/debots)

TIP3 tokens

[https://github.com/tonlabs/ton-labs-contracts/tree/master/cpp](https://github.com/tonlabs/ton-labs-contracts/tree/master/cpp)
[https://forum.freeton.org/t/tip-3-distributed-token-or-ton-cash/64](https://forum.freeton.org/t/tip-3-distributed-token-or-ton-cash/64)
