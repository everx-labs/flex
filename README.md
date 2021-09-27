<p align="center"><img src="./screenshots/flash2.svg" height="56" width="56"></p> 
<h1 align="center">Flash Exchange (FLEX)</h1>

Flex is a [FreeTon](https://freeton.org/discover) decentralized and distributed limit order book (DLOB) which takes a most common centralized exchange model: central limit order book (CLOB) and implements it on-chain via distributed smart contract model. 

- [Content Table](#content-table)
- [Flex DeBot ](#flex-debot)
- [Listing a new pair on Flex](#listing-a-new-pair-on-flex)

## Flex DeBot 
 [`FlexDebot`](https://github.com/tonlabs/flex/blob/main/debots/flexDebot/flexDebot.sol) -[DeBot](https://help.ton.surf/en/support/solutions/articles/77000397693-what-are-debots-) that helps buy/sell TIP3 tokens: 
 - queue all available pairs
- queue all price contracts in the pair and display current sell/buy order quantity
- show current book price
- create and send **Good-til-cancelled (GTC) order** for a selected token quantity for a selected Price
Since this will require to have a token wallet and to transfer the ownership of the user wallet to the Price Contract it should do all this just by asking user for a seed phrase or private key once
- display active user orders
- withdraw active user orders

Flex Debot address in Surf [
https://uri.ton.surf/debot/0:e57e4fa3f5ab829c12ce145ce24c0a9b4aa666a0e60d5b00f8bfd6aa1d90d64f](https://uri.ton.surf/debot/0:e57e4fa3f5ab829c12ce145ce24c0a9b4aa666a0e60d5b00f8bfd6aa1d90d64f)


<img src="./screenshots/flex_qr.png" height="200" width="200">

Tip3 Debot address in Surf [
https://uri.ton.surf/debot/0:883993b781db9494349dbf61418711636e52d9fc9991785420e2925337a91e40](https://uri.ton.surf/debot/0:883993b781db9494349dbf61418711636e52d9fc9991785420e2925337a91e40)

<img src="./screenshots/flex_tip3_qr.png" height="200" width="200">

Flex API address `0:f207fcab308c52a75226212d58dcfbc146cc7c1df1be16046200432f062a0822`

### Prerequisites: 

You can run debots in (Surf)[https://ton.surf/main] or latest (tonos-cli)[https://github.com/tonlabs/tonos-cli/].  install:
```
tondev tonos-cli install
```

#### TL;TR

Compile all DeBots using [`tondev`](https://github.com/tonlabs/tondev) tool:

```bash
tondev sol compile <debotName>.sol
```

This is Flex:
<img src="./screenshots/flash3.svg" height="56" width="56">


*(we thank [@eternalflow](https://github.com/eternalflow) for a picture of his dog!)*

## What Problem Are We Trying to Solve?

The biggest problem with decentralized exchanges is its speed of execution, lack of advanced trading strategies ability and complex management. Here we propose a decentralized trading engine and order book with low latency and guaranteed trade execution. It is flexible — allowing extendable strategies, extremely fast — providing immediate execution and settlement of an order and both decentralized and distributed. This allows Flex to perform at par with Free TON blockchain performance. For example an average execution across 128 threads will by 0.08 seconds at roughly 80,000 trading pairs messages per second throughput for one shardchain. It will take just 15 workchains to beat Binance performance and 150 workchains to compete with BATS exchange. Usage of DeBots make it super easy to add simple or advanced user interfaces.

Flex is a decentralized and distributed limit order book (DLOB) which takes a most common centralized exchange model: central limit order book (CLOB) and implements it on-chain via distributed smart contract model. Many have tried this approach before and failed (EtherDelta, DDEX, Radar Relay, etc.). Up until now the problem of creating an order book on chain were: the slow speed of execution and the possibility of front-running orders. Some are trying to solve this by moving to the fastest possible blockchain. But just having a fast blockchain is not enough because usually order books are pretty large and complex which takes time to operate on. Flex distributed atomic contracts solves this problem distributing the load on the order book down to single price of a single pair.

A central exchanges using CLOB such as NYSE, CBOT, Coinbase, Binance operates in a microsecond time frames.

That is how it looks:

<img src="./screenshots/CLOB.png"/>

## How orders are filled on CLOB?

The most common algorithm used is Price/Time priority, aka FIFO: all orders at the same price level are filled according to time priority; the first order at a price level is the first order matched.

Up until now there was a problem implementing this algorithm on a blockchain because transactions are usually prioritized by miners based on the gas fee they pay, allowing for front running the Time priority. Free TON does not have these problems.

### CLOB are hard to implement on a blockchain because:

The size of the state needed by an order book to represent the set of outstanding orders (e.g., passive liquidity) is large and extremely costly in the smart contract environment, where users must pay for space and compute power utilized.

[G. Wood et al., “Ethereum: A secure decentralised generalised transaction ledger,”
Ethereum project yellow paper, vol. 151, no. 2014, pp. 1–32, 2014.]

The matching logic for order books is often complicated as it must often support
several different order types (such as icebergs, good-till-cancel, and stop-limit orders

[M. Wyart, J.-P. Bouchaud, J. Kockelkoren, M. Potters, and M. Vettorazzo, “Relation
between bid–ask spread, impact and volatility in order-driven markets,” Quantitative
Finance, vol. 8, no. 1, pp. 41–57, 2008]

## How does FLEX solve this

Flex achieves load balancing and speed of execution by distributing the logic among many atomic smart contracts, while simultaneously transfering search and decision logic to user computers. All Order Book contracts addresses are deterministically calculated: the Trading Pair and its Price order contracts.

For instance TON- XTIP3 will resolve a Root of TON-XTIP3 trading pair. User then can calculate an address of a particular Price Contract by entering a Price. Code Hash=$TON-TIP3 Data=1.2 will calculate an address of a contract that currently trades a TON-TIP3 pair at 1.2 TON per 1 TIP3 token.

After performing the calculation of addresses user performs read operation: querying blockchain database with SDK over GraphQL. Since each user only calculates pairs and prices it needs the operation takes microsenconds and one query is almost instantaneous.

Let’s agree that a trading step should not be more than 0.01 (this information will be provided in the pair Root Contract data). User can now calculate a whole order book of this pair by entering all price steps around the target price or ask database for all addresses of that Price contract code hash. After calculating all price step contract addresses a user may try to retrieve all these contracts. If the contract does not exist means there are no orders of that price.


<img src="./screenshots/Exchange.png"/>


In order to execute a trade user can choose several strategies:
If the price contract does not exist, a User can create a "Good-til-cancelled (GTC) order" by deploying the $asset pair contract adding the Price as a contract data in the constructor and sending the required amount of money to cover the trade. For example passing a price of $0.55 TIP3 into constructor and sending 10,000 TONs to the contract will create a sell order of 10,000 TONs for $0.55 TIP3 per 1 TON in $TIP3-TON trading pair contract. Sending TIP3 tokens will create a buy order.

All matched orders in the contract are executed immediately. If the contract does not have any orders it will terminate itself, this is important as the existence of the Price Contract at the calculated address by itself means orders of that price exist. Therefore just by retrieving all deployed contracts by a contract code hash, a user can see a whole stack of the order book.

If an order "Good till cancelled" can not be executed in one transaction it should execute itself in several sending each time a message to itself.

**Cancel order** — a User can Cancel un-executed orders at any time.

Many strategies are possible but are not yet implemented:
If the contract for the order price exists the user can send a message with **Immediate or cancel (IOC)** order which will be executed immediately even partially or canceled if the trade can not be performed.

**Fill or kill (FOK)** will cancel the order if a full order amount is not available.

**Good-til-cancelled (GTC)** can be created in which case the order will stay in the contract until it is fulfilled by a counterparty (if any).

User can discover price and perform some conditional strategies either on a client side or by a smart contract, emulating a high frequency trading:

**A price**

User client can retrieve all messages for a trading pair contract and sort them by time to find the latest trades of that pair. Successful trade transactions should emit a message so it could be found later.

Instead of a client software an automatic execution could be programmed with a **paid subscription method** of an $Asset contract. The contract may send a message to other contracts that are subscribed to it once any trade is executed. Such service is payable by contracts subscribed to it to the owner of the Order payable upon Contract termination. The subscribed contract can then perform automatic strategies based on this information, connect Automated Market Maker smart contracts or Liquidity Pools.

The following order types will be added shortly:

**A market order** could be filled if User client software will search for latest transactions in the pair contracts to sell 10,000 TONs. When finding a deployed contract with best price starting from the latest executed user client software can retrieve the offered amount and send the Currency to this contract. Sending TONs to such contract will immediately execute the trade with the contract asset price for the sent amount.

**A limit order** is an order to buy or sell a security at a specific price or better. A buy limit order can only be executed at the limit price or lower, and a sell limit order can only be executed at the limit price or higher. Example: A user could submit a limit order for a specific amount and this order will only execute if the price of ABC stock is at that price or lower.

## Tip3 wallets workflow

To sell tip3 tokens, client should call new tip3 method `lendOwnership` with dest=<Price address> and payload=<params for sellTip3>. Tip3 wallet sends notification about provided ownership with its current balance, lend finish time and the payload.

Price will check sender address to be a correct tip3 wallet address. If checks succeeded, Price will process or enqueue this sell.
Tip3 ownership is provided with finish time, when ownership will return back to the original owner (in case of error in service).

To buy tip3 tokens, client should specify tip3 wallet in his ownership by address in `buyTip3` arguments.

<img src="./screenshots/Exchange2.png"/>

## TIP3 to TIP3 exchange 

https://github.com/tonlabs/flex/blob/main/flex/PriceXchg.hpp
https://github.com/tonlabs/flex/blob/main/flex/XchgPair.hpp

## Listing a new pair on Flex
 

1. First you need to deploy a FlexClient with the Flex DeBot.
2. After you deployed the FlexClient, you should configure it to deploy trading pair:

```bash
tonos-cli call <flex_client_address> setExtWalletCode "{\"ext_wallet_code\":\"<external_wallet_code>\"}" --sign <path_to_keys or seed_phrase> --abi FlexClient.abi
``` 
where:
<external_wallet_code> is the code of TONTokenWallet.tvc contract (`tvm_linker decode --tvc TONTokenWallet.tvc`);
 
```bash
tonos-cli call <flex_client_address> setFlexWrapperCode "{\"flex_wrapper_code\":\"<flex_wrapper_code>\"}" --sign <path_to_keys or seed_phrase> --abi FlexClient.abi
``` 
where:
<flex_wrapper_code> is the code of Wrapper.tvc contract (tvm_linker decode --tvc Wrapper.tvc);

3. Deploy a new tip3 token with the tip3 DeBot and save root_public_key, root_address, name, symbol and number of decimals for the next step.
4. Now you can deploy Wrapper for tip3 token:
 
```bash
tonos-cli call <flex_client_address> deployWrapperWithWallet "{\"wrapper_pubkey\":\"<wrapper_pubkey>\",\"wrapper_deploy_value\":\"1500000000\",\"wrapper_keep_balance\":\"1000000000\",\"ext_wallet_balance\":\"1000000000\",\"set_internal_wallet_value\":\"500000000\",\"tip3cfg\":{\"name\":\"<tip3_name>\",\"symbol\":\"<tip3_symbol>\",\"decimals\":\"<tip3_decimals>\",\"root_public_key\":\"<tip3_root_public_key>\",\"root_address\":\"<tip3_root_address>\"}}" --sign <path_to_keys or seed_phrase> --abi FlexClient.abi
```
where:
<wrapper_pubkey> is a newly generated public key;
<tip3_name> is the name of the new token, it can be recovered by `tonos-cli run <tip3_root_address> getName "{}" --abi RootTokenContract.abi`;
<tip3_symbol> is the symbol of the new token, it can be recovered by `tonos-cli run <tip3_root_address> getSymbol "{}" --abi RootTokenContract.abi`;
<tip3_decimals> is a possible number of decimal places in the amount of this token, it can be recovered by `tonos-cli run <tip3_root_address> getDecimals "{}" --abi RootTokenContract.abi`;
<tip3_root_public_key> is the root_public_key from the previous step, it can be recovered by `tonos-cli run <tip3_root_address> getRootKey "{}" --abi RootTokenContract.abi`;
<tip3_root_address> is the root address from the previous step.

5. Now deploy trading pair:
```bash
tonos-cli call <flex_client_address> deployTradingPair "{\"tip3_root\":\"<tip3_root>\",\"deploy_min_value\":\"<deploy_min_value>\",\"deploy_value\":\"<deploy_value>\",\"min_trade_amount\":\"<min_trade_amount>\",\"notify_addr\":\"<notify_addr>\"}" --sign <path_to_keys or seed_phrase> --abi FlexClient.abi
```
where:
<tip3_root> is the wrapper address from the previous step;
<deploy_min_value> is the min amount TONs required to deploy a new trading pair from `tonos-cli run <flex_address> getTonsCfg "{}" --abi Flex.abi`
<deploy_value> is deploy_min_value + 1 TON;
<min_trade_amount> is the minimal amount of tip3 in trade orders (lot);
<notify_addr> is the subscription address for the new trading pair.

* In case of a tip3-tip3 pair:

```bash
tonos-cli call <flex_client_address> deployXchgPair "{\"tip3_major_root\":\"<tip3_major_root>\","tip3_minor_root\":\"<tip3_minor_root>\",\"deploy_min_value\":\"<deploy_min_value>\",\"deploy_value\":\"<deploy_value>\",\"min_trade_amount\":\"<min_trade_amount>\",\"notify_addr\":\"<notify_addr>\"}" --sign <path_to_keys or seed_phrase> --abi FlexClient.abi
```
where:
<tip3_major_root> - wrapper for major tip3;
<tip3_minor_root> - wrapper for minor tip3.

Full Client Deploy Debot
`0:c58db77f5b66afc78a39747a57ec26f2f291b6874389df62367492f8d8be5ffa`

Pair Deploy Debot `0:58134bcd108d0968ed5d17717b9838cf072ed13f7912a1f743feac3df6523a60`

Tip3 Ext To Int Debot 
`0:aaf216e285b842f65144938e0a6fd1e2f02e33f1985094e324c8911404cd1e1d`

Wrapper Deploy Debot `0:97f4c980c69d535e03133b26d33c36349fa648006b95d39d3172cc9d0143a310`

Tip3 Debot `0:cf3b864c45247930b01acf6706c34c00f7a42b48479dbe433e6efd49d5e3744b`
