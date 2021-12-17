# Flex SDK API

Flex SDK API allows to develop Flex client apps that query Flex contracts directly on the blockchain via the methods described in this document.

## Flex API

```graphql
type Flex(root: address) {
	info(): TExchangeInfo,
	tokens(): TFlexToken[],
	getToken(?tokenSymbol: string, ?address: string): TFlexToken,

	pairs(): TPair[],
	getPair(pair_ticker: string, pair_address: string): TPair,
	
	market(?pair_ticker: string, ?pair_address: string): TMarket {
		pair: TPair,
		recentTrades():TTrade[],
		priceHistory(resolution, startTime, endTime, ...): TPriceHistory,
		orderbook: TOrderbook,
		volume: uint
		price: floar
		high_24h: float
		low_24h: float
	},

	user(pubkey: string): FlexUser {
		equity: uint
		orders(?market): TOrder,
		trades(?market): TTrade,
		wallets():TUserWallet[]
		tokens(): TUserToken[]
	},
	
	statistics(): TStatistics{
		volume: uint
		tvl: uint
	}
}

subscription {
	getBars(?symbol:string, ?symbol_address: string): TBar;
	getOrderbook(?symbol:string, ?symbol_address: string): TOrderbook;
	getTrades(?symbol:string, ?symbol_address: string): TTrade;
}
```

### info

```graphql
type Flex(root: address) {
  info(): TExchangeInfo
}
type TExchangeInfo {
{
	name: string # FlexOwnershipInfo
	address: address
	ever_cfg: TEverCfg
	deals_limit: uint
}

type TEverCfg{
# renamed it because there are no more crystals
{ # ... for executing tip3 transfer
  uint128 transfer_tip3;
  # ... to return tip3 ownership
  uint128 return_ownership;
  # ... to deploy tip3 trading pair
  uint128 trading_pair_deploy;
  # .. to send answer message from PriceXchg
  uint128 order_answer;
  # \brief ... to process processQueue function.
  # Also is used for buyTip3 / onTip3LendOwnership / cancelSell / cancelBuy estimations.
  uint128 process_queue;
  # ... to send notification about completed deal (IFlexNotify)
  uint128 send_notify;
}
```

Flex info query returns Flex DEX setup parameters used by its contract system.

These parameters are stored in flex root contract data (`root` parameter).&#x20;

It is deployed once when exchange is created and does not change over time. Whenever there are updates in any of exchange contracts, Flex is redeployed from scratch.

Because of this, flex contract has `deprecated` bool field which indicates if this version is the latest, and also has links to prev and next flex version addresses. Flex has a method that will deprecate it and specify `next_root_address` emitting the update event, so that backend can catch it and switch to another version.

### tokens

```graphql
type Flex(root: address) {
  tokens(): TToken
}

type TToken {
	address: string # Flex Tip3 root address (wrapper address)
	name: string, # Full name
	ticker: string,  # Token ticker, i.e. 'EVER'
	decimals: uint # defines the number of decimal places
	total_allocated: uint # in contract - total_granted field
	wallet_code_hash: string #flex tip3 wallet code hash
	approve_pubkey: string, # root_pubkey - pubkey that registered and approved this wrapper (from flex client contract)
	approve_address: string # root_owner - address that registered and approved this wrapper (from flex client contract)
	external_tip3_wallet: string # wallet that locks and stores all external tip3 tokens

}
```

This method returns all tokens listed on Flex.

Token information is stored in so-called wrapper contracts, that wrap original TIP3 contracts and represent a root contract that deploys Flex wallets for a certain Token.

Hash of Wrapper contract can be retrieved from Flex root contract and it is the same for all Flex wrappers. So, to retrieve a list of tokens, we need to filter accounts by wrapper code\_hash, decode them and get all the required information about tokens. See the `Token` type description.

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
    pairs(ticker_substr_filter: String, limit: Int, offset: Int): [FlexPair]
}

type FlexPair {
    """
    Flex Pair account address
    """
    address: String

    """
    Abbrevation used to identify pair.
    Derived from major and minor root tickers, i.e. 'EVER/SOL'
    """
    ticker: String

    """
    """
    major_token: TToken, 

    """
    """
    minor_token: TToken, 

    """
    The amount of price precision steps for 1 tick. Does not exist yet, will be added. 
    """
    min_move: uint 

    """
    The number of price decimal places. 
    """
    price_denum: uint
		"""
    Min amount of major token required for an order creation in minimum token units. 
		
    """
    min_amount: uint,

    """
    Code hash of price contracts for the pair
    """
    price_code_hash: string 
    """
    Code of price contracts for the pair
    """
    price_code: string 

    """
    notification contract address
    """
    notify_addr: string
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
}r
```



This query retrieves a list of trading pairs on Flex.

All Trading Pair contracts have the same code hash that is stored in Flex Root contract.

Later, to query Flex Orderbook, we will need to collect all Price contracts (which accept, store and process buy/sell orders from users). Price contract hash code can be calculated from Flex Root getmethod (_it is not stored in Flex Root data, calculated with getmethod only_). Each Trading Pair has its own Price Contract hash.

### getPair

```graphql
type Flex(root: address) {
	getPair(pair_ticker: string, address: string): TSymbol,
}
```

Query that returns info of a particular pair by its Trading Pair contract address or Pair string.

### Market

Market - all the data, relevant for 1 Trading Pair.

All queries inside a Market return data filtered by the relevant `pair_ticket` string or address

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
		pair: TPair,
		recentTrades():TTrade[],
		orderbook: TOrderbook,
		getBars(resolution, start_time, end_time,...): TBar[], # bars
		statistics: TMarketStats {
			TVL: uint,
			volume24h: uint
		}
}
```

#### pair

```graphql
type MarketData(?pair_ticket: string, ? pair_address: string){
		pair: TPair,
}
```

Returns the same data as Flex.getPair query.

#### recentTrades

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
		recentTrades():TTrade[]
}

type TTrade {
	side: enum { SELL, BUY }
	amount: float, # = sum_deals_amount_ / 10^(MajorToken.decimals)
	price: float, # = (price_num/price_denum) * 10^(major.decimals-minor.decimals)

	price_num: uint,
	price_denom: uint,
	amount_units: uint
	time: timestamp, 
}
```

This query allows to get 50 recent trades of a Trading Pair.

When user wants to trade one token for another, they send an order to Price Contract.

☝🏻 For each Trading Pair there are many Price Contracts for each Trading Pair price. These contracts aggregate and process orders with that price.

An order has such fields as amount and expiration time. One order can be fulfilled fully or partially with 1 or more trades.

Whenever an order is fully or partially executed (i.e. trade has happened), Price Contract sends an `onXchgDealCompleted` message to Trading Pair Notification Contract (`pair.notify_addr`).

So, to collect all trades for a Trading Pair we need to query all `onXchgDealCompleted` messages and parse them.

```graphql
if (trade_price > prev_trade_price) then side = "buy", else side = "sell"ph
```

#### priceHistory

Price and volume history is needed by trading applications to draw bars or candles.

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
	getBars(
	resolution: TResolution, # Enum, in seconds, i.e. 60, 300, 3600, 86400 
	start_time: timestamp, # (inclusive end)
	end_time: timestamp, # (not inclusive)
	# this field defines minimum number of returned bars, and has priority over start_time parameter, i.e. you must return data in the range `[from, to)`, but the number of bars should not be less than `countBack`.
	countBack: uint,
	# boolean to identify the first call of this method.
  # When it is set to `true` you can ignore `to` (which depends 
	# on browser's `Date.now()`) and return bars up to the latest bar
	firstDataRequest: bool): PriceHistory { 
		TPrice[], 
		TBarMeta
	} # bars
}
type TPrice {
	symbol_address: address,
	open: float,
	close: float,
	low: float,
	high: float,
	time: datetime
	volume: float
}
type meta: TPriceMeta {
# This flag should be set if there is no data in the requested period.
		noData: bool 
# Time of the next bar in the history. It should be set if the requested period represents a gap in the data. Hence there is available data prior to the requested period.
		nextTime: timestamp 
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

> **Example 1**: let's say the chart requests 300 bars with the range `[2019-06-01T00:00:00..2020-01-01T00:00:00]` in the request. If you have only 250 bars in the requested period (`[2019-06-01T00:00:00..2020-01-01T00:00:00]`) and you return these 250 bars, the chart will make another request to load 50 more bars preceding `2019-06-01T00:00:00` date.
>
> **Example 2**: let's say the chart requests 300 bars with the range `[2019-06-01T00:00:00..2020-01-01T00:00:00]` in the request. If you don't have bars in the requested period, you don't need to return `noData: true` with the `nextTime` parameter equal to the next available data. You can simply return 300 bars prior to `2020-01-01T00:00:00` even if this data is before the `from` date.



#### orderbook

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
		orderbook():TOrderBook
}
	type TOrderBook(limit: uint) {
		bids: OBItem[],
		asks: OBItem[],
	}

	type OBItem{
		price : float, # price of 1 major token denominated in minor token with precision of minor.decimals: price * 10^(major.decimals-minor.decimals)
		amount : float # amount of major token with Pair.token.decimals precision: amount_in_contract / 10^(Pair.MajorToken.decimals)
		price_num: uint,
		price_denum: uint,
		amount_units: uint
	}
```

Orderbook is a list of available Trading Pair pools for buying and selling aggregated by price:

Each Price Contract has information about available amount for buying and selling of a particular Symbol (Trading Pair) for that price.

So, in order to calculate an orderbook for a Market, we need to collect all Price Contracts and parse their data to retrieve sell amount, buy amount and the price itself and aggregate this data.

`bid` - buys

`ask` - sells

Bids are sorted by price, descending.

Asks are sorted by price, ascending.

Return top 50 bids and asks.

#### Volume

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
    price: Float,
    last24H: FlexPriceSummary,
}
```

Provides a view of rolling 24 hour volume on the Flex for the market.

Volume is calculated as a sum of all trade volumes for the last 24 hours.

#### Price, high\_24, low\_24

```graphql
type Market(?pair_ticket: string, ? pair_address: string){
			price: float
			high_24h: float
			low_24h: float
}
```

Latest trade's price, lowest and highest price for the last 24 hours.

### User data

```graphql
type UserData(?pubkey: string): TUserData {
		wallets(): TUserWallet[],
		tokens(): TUserToken[],
		equity: uint
		orders(?market): TOrder,
		trades(?market): TTrade,
	}
```

#### equity

Equity - total balance of all tokens converted to USC currency price.

```graphql
type UserData(pubkey: string): TUserData {
		equity: uint  # 
	}
```

#### wallets

```graphql
type UserData(pubkey: string): TUserData {
		wallets(): TUserWallet[]
	}

type TUserWallet {
	wallet_address: address
	token: TToken, # by wrapper pubkey
	balance: uint, # in minimum token fraction
	balance_in_orders: uint # sum of landed ownerships for price contracts addresses
	landed_ownerships: TLandedOwnership[] 
}

type TLandedOwnership{
	address: address,
	pubkey: string
	balance: uint
	finish_time: timestamp
}
```

#### tokens

```graphql
type UserData(pubkey: string): TUserData {
		tokens(): TUserToken[]
	}

type TUserToken {
	wallet: TUserWallet
	type: enum {OWN, LANDED}
	balance: uint, # amount that the pubkey controls
	finish_time: ?timestamp, # filled only for landed ownerships
	balance_in_orders: uint # NULL
}
```

#### orders

```graphql
type UserData(pubkey: string): TUserData {
		orders(?market): TOrder,
	},
type TOrder{
	pair: TPair
	order_id: string
	type: enum {POST}
	amount: decimal
	price: decimal
	status: enum {active, calceled, partially_executed, fully_executed}
	last_updated: datetime
}
```

#### trades

```graphql
type UserData(?pubkey: string, ?client_address: string): TUserData {
		trades(?market): TOrder,
	},
type TOrder{
	pair: TPair
	amount: decimal
	price: decimal
	time: datetime
}
```

### statistics

```graphql
type Flex(root: address) {
		statistics: TMarketStats {
			TVL: uint # total value locked in Flex denominated in ??? currency or Token
			volume: uint # 24 hour volume on the Flex for all markets.
			price_high: float,
			price_low: float,
			
		}
}
```
