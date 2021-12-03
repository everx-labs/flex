# Economic Model

There are essentially two profit centers in the Flex Ecosystem:

* Flex Exchange Root
* Flex AMM

## Flex Exchange Root

The **Flex Exchange Root** collects a configurable percentage fee from both buyer and seller for processing transactions in the Order Book. Governance Token holders control the percentage.

## Flex AMM

The **Flex AMM** is a market maker, i.e. by providing liquidity to the Order Book it facilitates the process of finding a fair price for a pair of tokens and ensures that market participants have a trading partner. If the fair price is found, the market maker would earn the price difference between their bid and ask prices when participants trade against them. The market maker may also earn a rebate fee from the exchange for providing liquidity.

## Liquidity Providers

Liquidity Providers contribute tokens to the Flex AMM Pools to get a share of the Flex AMM profits. On exit from the Flex AMM Pools, they receive their share of the total value locked in the Pool.

The current implementation of AMM calculates the share of the Pool participants in TVL on a per trading session basis without issuing liquidity tokens. However, the implementation can be modified with the addition of liquidity tokens that are issued in exchange for the provided liquidity and burned when leaving the Pool. Such liquidity tokens can in turn be traded on exchanges, used for farming, etc.
