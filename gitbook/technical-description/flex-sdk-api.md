# Flex API

Flex API allows to develop Flex client apps that query Flex contracts directly on the blockchain via the methods described in this document.

## Flex API

```graphql
# Query
type Query {
    flex(root: String): Flex
}

type Flex {
    info: FlexInfo
		migrations: FlexMigration[],
    tokens(
        tickerSubstringFilter: String,
        limit: Int,
        offset: Int
    ): [FlexToken]
    getToken(tokenTicker: String, tokenAddress: String): FlexToken

    pairs(
        tickerSubstringFilter: String,
        limit: Int,
        offset: Int
    ): [FlexPair]
    getPair(pairTicker: String, pairAddress: String): FlexPair
    market(pairTicker: String, pairAddress: String): FlexMarket
}
```

### migrations

```graphql
type FlexMigration {
	update_no: UInt
	address: string # config contract address

	# all the data below is parsed from Config Contract data
	wallet_group_version: UInt
	exchange_group_version: Uint
	user_group_version: UInt
	root_addr: string,
	user_config_addr: string
}
```

Parse Super Root contract for migration sequence and Config Contracts for each migration version

### info

```graphql
type FlexInfo {
    name: String # FlexOwnershipInfo
    address: String
    feesConfig: FeesConfig
    listingConfig: ListingConfig
}

type FeesConfig {
    ...
}
```

Flex info query returns Flex DEX setup parameters used by its contract system. wrapper\_code\_hash, pair\_code\_hash

These parameters are stored in flex root contract data (`root` parameter).&#x20;

It is deployed once when exchange is created and does not change over time. Whenever there are updates in any of exchange contracts, Flex is redeployed from scratch.

### tokens

```graphql
type FlexToken {
    # Flex Tip3 root address (wrapper address)
    address: String
    # Full name
    name: String
    # Token ticker, i.e. 'EVER'
    ticker: String
    # defines the number of decimal places
    decimals: Int
    # total_granted in tokens
    totalAllocated: Float
    # in contract - total_granted field in units
    totalAllocatedUnits: String
    #flex tip3 wallet code hash
    walletCodeHash: String

		wrapperType: enum {TIP3, WEVER, native(NEVER, etc),..., BroxusTip3}

****    # Account that locks and stores all external tokens
    # for WEVER - empty
    externalAddress: String
}
```

This method returns all tokens, listed on Flex.

Token information is stored in so-called wrapper contracts, that wrap original tip3 contracts and represent a root contract that deploys Flex wallets for a certain Token.

Code of wrapper contract is here

Hash of Wrapper contract can be retrieved from Flex root contract and it is the same for all Flex wrappers. So, to retrieve a list of tokens, we need to filter accounts by wrapper code\_hash, decode them and get all the required information about tokens. See the `Token` type description.

Flex root has hash code of wEver wrapper.

### getToken

```graphql
type Flex(root: address) {
	getToken(?ticker: string, ?address: string): TToken,
}
```

Query that returns info of a particular token by its wrapper address or token symbol string.

### pairs

```graphql
type Flex {
    pairs(
        tickerSubstringFilter: String,
        limit: Int,
        offset: Int
    ): [FlexPair]
}

type FlexPair {
    " Flex Pair account address "
    address: String

    """
    Abbrevation used to identify pair.
    Derived from major and minor root tickers, i.e. 'EVER/SOL'
    """
    ticker: String

    " Major token of the pair "
    major: FlexToken

    " Minor token of the pair "
    minor: FlexToken

		"""
    Min amount of major token required for an order creation
    in token units.
    """
    minAmount: Float

		" minAmount in major token units "
    minAmountUnits: String

    " Code hash of price contracts for the pair "
    priceCodeHash: String

    " notification contract address "
    notifyAddress: String

		**NEW!! price_scale: number,
		NEW!! price_scale_units: number,
		NEW!! min_move: number,**
    price_code_hash: string,
}
```

Query example:

```graphql
# Request

query {
   flex(root: "0:11111") {
      info { name }
      getPair(ticker: "EVER/USD") {
         majorToken {
            name
         }
         minorToken {
            name
         }
      }
   }
}

# Response

{
    "info": { "name": "TONLabs Flex Exchange" },
    "getPair": {
        "majorToken": { "name": "Everscale" },
        "minorToken": { "name": "US Dollar" }
    }
}
```

This query retrieves a list of trading pairs on Flex.

All Trading Pair contracts have the same code hash that is stored in Flex Root contract.

Later, to query Flex Orderbook, we will need to collect all Price contracts (which accept, store and process buy/sell orders from users). Price contract hash code can be calculated from Flex Root getmethod (_it is not stored in Flex Root data, calculated with getmethod only_). Each Trading Pair has its own Price Contract hash.

**Tick size definition:** `tick_size (MinimalPossiblePriceChange) = minmov / pricescale`, where

`pricescale` defines the number of decimal places in the quote currency (minor token). It is 10^number-of-decimal-places. If a price is displayed as 1.01, pricescale is 100; If it is displayed as 1.005, pricescale is 1000.

`minmov` is the amount of price precision steps for 1 tick.

### getPair

Query that returns info of a particular Symbol (pair) by its Trading Pair contract address or Pair Symbol string.

### Market

Market - all the data, relevant for 1 Trading Pair.

All queries inside a Market return data filtered by the relevant `pair_ticket` string or address

```graphql
type Market {
    pair: FlexPair
    orderbook(limit: Int): FlexOrderbook
    recentTrades(after: String): [FlexTrade]
    priceHistory(
        resolution,
        startTime, endTime,...
    ): [FlexPriceSummary], # bars
}
```

#### recentTrades

```graphql
type FlexTrade {
    side: TradeSide
    time: Int

    sellToken: FlexToken
    buyToken: FlexToken

    # = sum_deals_amount_ / 10^(MajorToken.decimals)
    amount: Float
    amountUnits: String

    # = (price_num/price_denum) * 10^(major.decimals-minor.decimals)
    price: Float
    priceNum: String
    priceScale: String
    priceScaleUnits: String
    cursor: String
}
```

This query allows to get 50 recent trades of a Trading Pair.

When user wants to trade one token for another, they send an order to Price Contract.

For each Trading Pair there are many Price Contracts for each price. These contracts aggregate and process orders with that price.

An order has such fields as amount and expiration time. One order can be fulfilled fully or partially with 1 or more trades.

Whenever an order is fully or partially executed (i.e. trade has happened), Price Contract sends an `onXchgDealCompleted` message to Trading Pair Notification Contract (`pair.notify_addr`).

So, to collect all trades for a Trading Pair we need to query all `onXchgDealCompleted` messages and parse them.

`side` field is Taker's side in the trade, now we implement artificial algorithm:

```graphql
if (trade_price > prev_trade_price) then side = "buy", else side = "sell"
```

#### priceHistory

Price and volume history is needed by trading applications to draw so-called bars or candles.

```graphql
type Market {
    priceHistory(
        # in seconds, i.e. 60, 300, 3600, 86400 
        resolution: Int
        # (inclusive end)
        startTime: Int
        # (not inclusive)
        endTime: Int
        # this field defines minimum number of returned bars,
        # and has priority over start_time parameter,
        # i.e. you must return data in the range `[from, to)`,
        # but the number of bars should not be less than `countBack`.
        countBack: Int
    ): FlexPriceHistory
}

type FlexPriceHistory {
    prices: [FlexPriceSummary]
    nextTime: Int
}

type FlexPriceSummary {
    time: Int
    open: Float
    close: Float
    low: Float
    high: Float
    volume: Float
}
```

This data is collected from trades. Each dot on the graph should have such data as open, close, highest and lowest price and volume for each period starting from `start_time`. Period length is defined by `resolution` parameter.

**Prices in contracts represent price of major minimum fraction for an amount of minor minimum fraction. We convert them to the price of 1 Major Token in X Minor Tokens.**

Use this formula:

```graphql
price = (price_num/price_denum) * 10^(major.decimals-minor.decimals)
volume = (total amount)/ 10^(Pair.MajorToken.decimals)
```

From and to need to be aligned by resolution.

We do not show `getEmptyBars` parameter and assume it as false. We do not return empty bars.

📚 Example 1: let's say the chart requests 300 bars with the range `[2019-06-01T00:00:00..2020-01-01T00:00:00]` in the request. If you have only 250 bars in the requested period (`[2019-06-01T00:00:00..2020-01-01T00:00:00]`) and you return these 250 bars, the chart will make another request to load 50 more bars preceding `2019-06-01T00:00:00` date.

Example 2: let's say the chart requests 300 bars with the range `[2019-06-01T00:00:00..2020-01-01T00:00:00]` in the request. If you don't have bars in the requested period, you don't need to return `noData: true` with the `nextTime` parameter equal to the next available data. You can simply return 300 bars prior to `2020-01-01T00:00:00` even if this data is before the `from` date.

#### orderbook

```graphql
type Market {
    orderbook(limit: Int): FlexOrderBook
}

type FlexOrderBook {
		bids: [FlexOrderbookItem]
		asks: [FlexOrderbookItem]
	}

type FlexOrderbookItem {
    # pric of 1 major token denominated in minor token with precision 
    # of minor.decimals:
    # priceNum/price_denom * 10^(major.decimals-minor.decimals)
		price : Float
		priceNum: String
		priceScale: String
		priceScaleUnits: String
    # amount of major token with Pair.token.decimals precision:
    # amount_in_contract / 10^(Pair.MajorToken.decimals)
		amount: Float
		amountUnits: String
}
```

Orderbook is a list of available Trading Pair pools for buying and selling aggregated by price.

Each Price Contract has information about available amount for buying and selling of a particular Pair for that price.

So, in order to calculate an orderbook for a Market, we need to collect all Price Contracts and parse their data to retrieve sell amount, buy amount and the price itself and group this data by price.

bid - buys

ask - sells

bids are sorted by price DESC

asks are sorted by price DESC

Return top 50 bids and asks.

#### Statistics

```graphql
type Market {
    # Latest trade's price
    price: Float
    # price and volume stats for the last 24 hours
    last24H: FlexPriceSummary
}
```

## User data

[Glossary](glossary.md)

#### wallets

```graphql
type Flex(root: address) {
    wallets(
        clientAddress: String!,
        userId: String,
        dappPubkey: String,
        after: String
    ): FlexWallet[]
} 

type FlexWallet {
    # flex wallet address
    address: String
    # flex client contract address
    clientAddress: String
    # walletPubkey
    userId: String
    #
    dappPubkey: String 
    # by root_address_ (wrapper)
    token: FlexToken
    #
    lendFinishTime: Int

    # balance of native currency of the wallet (in nanoevers)
    nativeCurrencyBalance: Float
    #
    nativeCurrencyBalanceUnits: String
    # balance_ field
    totalBalance: Float
    # balance_ field // Token balance of the wallet
    totalBalanceUnits: String
    # balance_- lended_owners balance
    availableBalance: Float
    #
    availableBalanceUnits: String
    #
    balanceInOrders: String
		balanceInOrdersUnits: String

    # Cursor that can be used for pagination
    cursor: String
}
```

All user wallets are salted with flex root address, so we return all wallets that belong to this root.

Flex wallets are deployed by Flex Client contract which becomes their owner. So, `owner_address_` is flex client address. Therefore, we need to use flex client addresses to search for all the client’s wallets.

Flex Client can lend some amount of tokens to another user with userId = contract.wallet\_pubkey for some period of time = lendFinishTime.

#### userOrders

[Glossary](glossary.md)

```graphql
type Flex {
    userOrders(userId: String): [FlexUserOrder]
}

type FlexUserOrder {
    pair: FlexPair
    side: FlexTradeSide
	  amountLeft: Float
	  amountLeftUnits: String
	  amountProcessed: Float
    amountProcessedUnits: String
    price: Float
    priceNum: String
    priceScale: String
    priceScaleUnits: String
    ****userId: String
    orderId: String
    finishTime: Int
****}
```

Sort by price ASC, order\_id. Returns top 50 records.

In this API we show only not executed or partially executed orders that sit in Price contracts. You can retrieve them by get methods getBuys, getSells.

#### userTrades

```graphql
type Flex {
    userTrades(
        userId: String
        limit: Int
        offset: Int
    ): [FlexUserTrade]
}

enum FlexTradeLiquidity {
    TAKER
    MAKER
}

type FlexUserTrade {
    pair: FlexPair
    side: FlexTradeSide
    amount: Float
    amountUnits: String
    price: Float
    priceNum: String
    priceScale: String
    priceScaleUnits: String
    time: Int
    liquidity: FlexTradeLiquidity
    fees: Float
    feesUnits: String
    cursor: String
}
```

User trades list returns all user trades history on all markets.

Retrieved from onTip3Transfer(balance\_, tokens, sender\_pubkey, sender\_owner, payload, answer\_addr, price\_num, price\_denom ) event of Flex Wallets sent to Flex Client

#### userEquity

Equity - total balance of all tokens converted to EVER currency price

```graphql
type userEquity(pubkey: string): TUserData {
		equity: uint  # 
	}
```

### Flex statistics (not implemented yet)

We will be able to do it only when we have USD xTip3, we will then calculate all tokens prices denominated in USD and sum them up to get TVL.

```graphql
type Flex(root: address) {
		statistics: TMarketStats {
			TVL: uint # total value locked in Flex denominated in ??? currency or Token
			volume: uint # 24 hour volume on the Flex for all markets.		
		}
}
```

## Subscriptions

#### Orderbook, recentTrades, priceHistory

```graphql
input MarketPair {
    ticker: String
    address: String
}

subscription {
    """
    Returns market orderbook and after that whenever there is a change 
    to orderbook, sends the updated Orderbook.
    """
    flexMarketOrderBook(
        flex: String,
        pairAddress: String,
        pairTicker: String
    ): FlexOrderBook

    """
    Starts sending new trades 
    created after trade specified by after_id.
    """
    flexMarketRecentTrades(
        flex: String,
        pairAddress: String,
        pairTicker: String
				after: string
    ): FlexTrade[]

    """
		Returns an array of price summary objects strting from
		start_time aggregated by resolution and then 
		starts sending price summary object whenever there is a new trade
		for the last time range defined by resolution.
		Timerange is changing over time. If resolution is 1 minute, then, when 
    the minute passes, a new timerange is started and new priceSummary object is returned.
    """
    flexMarketPriceHistory(
        flex: String, 
        pair_address: String,
        pair_ticker: String,
				start_time: uint,
        resolution: Int
    ): FlexPriceSummary[]
}
```

#### userOrders, userTrades, wallets

```jsx
subscription {
    """
    Returns market orderbook and after that whenever there is a change 
    to orderbook, sends the updated Orderbook.
    """
    flexUserOrders(
        flexAddress: String,
        userId: String
    ): FlexUserOrder[]

    """
    Starts sending new trades 
    created after trade specified by after cursor.
    """
    flexUserTrades(
        flexAddress: String,
        userId: String,
				after: String
    ): FlexTrade[]

    """
		Returns list of client wallets then starts sending 
    new wallet creations and existing wallet updates.
		Wallet deletions are not included in event types.
		if after is not specified, starts sending only 
		latest updates since subscription creation
    """
    flexWallets(
        flex: String,
        clientAddress: String!,
        userId: String,
				dappPubkey: String,
        after: String # after cursor
    ): FlexWallet[]
}
```
