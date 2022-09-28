# Overview

Flex is based on two simple concepts - distributing all trading logic over standardized easily searchable by `code_hash` contracts and deterministic calculating of addresses of these contracts based on their function in the system.

Flex consists of the following contracts:

* Flex Root
* Wrapper (Flex TIP3 token root)
* Exchange Pair
* Price exchange
* Flex Client
* Flex TIP3 token wallets

View source code [here](https://github.com/tonlabs/flex/tree/main/flex) and documentation [here](https://tonlabs.github.io/flex/).

## Flex Contracts

### Flex Root

Flex Root contract contains exchange configuration: Exchange Pair and Wrapper contract code hash, which are required to identify contracts belonging to the current implementation of Flex.

### Wrapper (TIP3 Root)

Wrapper contract is the primary contract for any token listed on Flex. It converts external tokens into Flex TIP3 tokens, deploys Flex TIP3 wallets, stores relevant token information, including Flex TIP3 wallet code hash.

### Exchange pair

Exchange pair contract defines a trading pair, deploys price exchange contracts and stores pair information, including notification subscription address and Price exchange code hash, which is calculated from Flex Root get method. To optimize search each trading pair has a unique Price exchange code hash by which all Price contracts of this pair may be located.

### Price Exchange

Price exchange contract enables trading of a pair at a specific price. It is deployed when a user creates a buy or sell order. Its address is deterministically calculated from trading pair and price value, thus allowing to locate a specific price contract without querying the blockchain. Multiple Price exchange contracts comprise the Order book for a trading pair.

### Flex Client

Flex Client governs contracts owned by user in Flex. It is deployed when the user first arrives on Flex and serves as an interface between the user and the Flex ecosystem. It initiates TIP3 wallet deployment, allows to list new tokens and place orders (initiates deployment of Price exchange contracts).

### Flex TIP3 Wallet

Flex TIP3 wallets store user tokens within Flex. Users interact with their wallets via Flex Client contract, may deposit or withdraw their tokens. For the purposes of fulfilling trading orders ownership of Flex TIP3 Wallets may be temporarily lent to Price Exchange contracts. To support a variety of exchange use cases, ownership may also be lent to [authorized](client-app-authorization.md) applications.

