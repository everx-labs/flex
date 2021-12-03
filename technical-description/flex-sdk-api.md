# Flex SDK API

Flex SDK API allows to develop Flex client apps that query Flex contracts directly on the blockchain via the methods described in this document.

## Flex API

```graphql
type Flex(root: address) {

  info(): TFlexInfo,

	tokens(): TToken[],
	getToken(?tokenSymbol: string, ?address: string): TToken,

	symbols(): TSymbol[],
	getSymbol(symbol: string, address: string): TSymbol,
	
	marketData(?symbol:string, ?symbol_address: string): TMarketData {
		symbol: TSymbol,
		recentTrades():TTrade[],
		getBars(resolution, start_time, end_time, ?limit): TBar[], # bars
		orderbook: TOrderbook,
		volume: uint
	},

	userData(?pubkey: string, ?address: string): TUserData {
		equity: uint
		orders(?market): TOrder,
		trades(?market): TTrade,
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
	**version: string # Flex version # now hardcode manually as 0.1, will be added in the next flex version**
	address: string # flex contract address
	wrapper_code_hash: string, 
	pair_code_hash: string,
	**deprecated: bool # synthetic field, does not exist. we will mark it as true in database whenever new flex is deployed. So, set to false by default. Will update it manually at first, will automate it after this field appears in the next Flex version
	next_root_address: address # address of the next flex version
	prev_root_address: address # address of the previous flex version
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
	tokenSymbol: string,  # Token symbol, i.e. 'EVER'
	decimals: uint # defines the number of decimal places
	total_allocated: uint # in contract - total_granted field
	wallet_code_hash: string #flex tip3 wallet code hash
	approve_pubkey: string, # root_pubkey - pubkey that registered and approved this wrapper (from flex client contract)
	approve_address: string # root_owner - address that registered and approved this wrapper (from flex client contract)
	external_tip3_wallet: string # wallet that locks and stores all external tip3 tokens
}
```

This method returns all tokens, listed on Flex.

Token information is stored in so-called wrapper contracts, that wrap original TIP3 contracts and represent a root contract that deploys Flex wallets for a certain Token.

Hash of Wrapper contract can be retrieved from Flex root contract and it is the same for all Flex wrappers. So, to retrieve a list of tokens, we need to filter accounts by wrapper code\_hash, decode them and get all the required information about tokens. See the `Token` type description.

### getToken

```graphql
type Flex(root: address) {
	getToken(?tokenSymbol: string, ?address: string): TToken,
}
```

Query that returns info of a particular token by its wrapper address or token symbol string.

### symbols

```graphql
type Flex(root: address) {
	symbols(): TSymbol[],
}

type TSymbol {
	address: string # Flex Pair contract address
	**symbol: string,  # Calculated from major and minor root tokenSymbols, i.e. 'EVER/SOL'
	type: Enum {stock} # Hardcode = 'stock', does not exist in the contract.**
	major_root: address, 
	minor_root: address,
	session: string, #Hardcoded. Always "24x7"
	# The amount of price precision steps for 1 tick. For example, since the tick size for U.S. equities is `0.01`, `minmov` is 1. But the price of the E-mini S&P futures contract moves upward or downward by `0.25` increments, so the `minmov` is `25`.
	**minmov: uint # Temporary hardcode = 1, does not exist yet, will be added.**
	pricescale: uint # Token.decimals of minor token. **Should add it to so that we do not need to search for it.**
	min_amount: uint,
	price_code_hash: string # Code has of price contracts for the pair
	exchange: string # flex address 
	notify_addr: string #notification contract address
}
```

This query retrieves a list of trading pairs (aka symbols) on Flex.

All Trading Pair contracts have the same code hash that is stored in Flex Root contract.

Later, to query Flex Orderbook, we will need to collect all Price contracts (which accept, store and process buy/sell orders from users). Price contract hash code can be calculated from Flex Root getmethod (_it is not stored in Flex Root data, calculated with getmethod only_). Each Trading Pair has its own Price Contract hash.

### getSymbol

```graphql
type Flex(root: address) {
	getSymbol(symbol: string, address: string): TSymbol,
}
```

Query that returns info of a particular Symbol (pair) by its Trading Pair contract address or Pair Symbol string.

### Market

Market - all the data, relevant for 1 Trading Pair (Symbol).

All queries inside a Market return data filtered by the relevant Symbol string or address

```graphql
type Market(?symbol: string, ? symbol_address: string){
		symbol: TSymbol,
		recentTrades():TTrade[],
		orderbook: TOrderbook,
		getBars(resolution, start_time, end_time, ?limit): TBar[], # bars
		statistics: TMarketStats {
			TVL: uint,
			volume24h: uint
		}
}
```

#### symbol

```graphql
type Market(?symbol: string, ? symbol_address: string){
		symbol: TSymbol,
}
```

Returns the same data as `Flex.getSymbol` query.

#### recentTrades

```graphql
type Market(?symbol: string, ? symbol_address: string){
		recentTrades():TTrade[]
}

type TTrade {
	isSell: bool
	symbol: string, # i.e. 'EVER/USD' 
	symbol_address: address, # i.e. 'EVER/USD' 
	amount: int,
	price: uint, # per amount of major token = 1
	decimals: uint # of minor token
	time: timestamp,
	**buy_order_id: string # will be added later
	sell_order_id: string # will be added later
	buy_order_type: string # this field will be calculated from the first bits of order_id
	sell_order_type: string # this field will be calculated from the first bits of order_id
	buyer_flex_client: string # to identify buyer user
	seller_flex_client: string # to identify seller user
	fee_cost: 
}
```

This query allows to get 50 recent trades of a Trading Pair.

When user wants to trade one token for another, they send an order to Price Contract.

☝🏻 For each Trading Pair there are many Price Contracts for each Trading Pair price. These contracts aggregate and process orders with that price.

An order has such fields as amount and expiration time. One order can be fulfilled fully or partially with 1 or more trades.

Whenever an order is fully or partially executed (i.e. trade has happened), Price Contract sends an `onXchgDealCompleted` message to Trading Pair Notification Contract (`Symbol.notify_addr`) (See Buy/Sell diagram).

So, to collect all trades for a Trading Pair we need to query all `onXchgDealCompleted` messages and parse them.

#### orderbook

```graphql
type Market(?symbol: string, ? symbol_address: string){
		orderbook():TOrderBook
}
	type TOrderBook {
		bids: OBItem[],
		asks: OBItem[],
		symbol_address: address ?
	}

	type OBItem{
		price : float, # price of 1 major token denominated in minor token
		amount : float
	}
```

Orderbook is a list of available Trading Pair pools for buying and selling aggregated by price:

Each Price Contract has information about available amount for buying and selling of a particular Symbol (Trading Pair) for that price.

So, in order to calculate an orderbook for a Market, we need to collect all Price Contracts and parse their data to retrieve sell amount, buy amount and the price itself and aggregate this data.

#### getbars

Price and volume history is needed by trading applications to draw so-called bars or candles.

```graphql
type Market(?symbol: string, ? symbol_address: string){
	getBars(
	resolution: uint, # in seconds, i.e. 60, 300, 3600, 86400 
	start_time: timestamp, # (inclusive end)
	end_time: timestamp, # (not inclusive)
	# this field defines minimum number of returned bars, and has priority over start_time parameter, i.e. you must return data in the range `[from, to)`, but the number of bars should not be less than `countBack`.
	countBack: uint,
	# boolean to identify the first call of this method.
  # When it is set to `true` you can ignore `to` (which depends 
	# on browser's `Date.now()`) and return bars up to the latest bar
	firstDataRequest: bool): TBars { 
		TBar[], 
		TBarMeta
	} # bars
}
type TBar {
	symbol_address: address,
	open: float,
	close: float,
	low: float,
	high: float,
	start_time: datetime
	volume: float
}
type meta: TBarMeta {
# This flag should be set if there is no data in the requested period.
		noData: bool 
# Time of the next bar in the history. It should be set if the requested period represents a gap in the data. Hence there is available data prior to the requested period.
		nextTime: timestamp 
	}
```

This data is collected from trades. Each dot on the graph should have such data as open, close, highest and lowest price and volume for each period starting from `start_time`. Period length is defined by `resolution` parameter.

📚 Example 1: let's say the chart requests 300 bars with the range \`\[2019-06-01T00:00:00..2020-01-01T00:00:00]\` in the request. If you have only 250 bars in the requested period (\`\[2019-06-01T00:00:00..2020-01-01T00:00:00]\`) and you return these 250 bars, the chart will make another request to load 50 more bars preceding \`2019-06-01T00:00:00\` date.

Example 2: let's say the chart requests 300 bars with the range `[2019-06-01T00:00:00..2020-01-01T00:00:00]` in the request. If you don't have bars in the requested period, you don't need to return `noData: true` with the `nextTime` parameter equal to the next available data. You can simply return 300 bars prior to `2020-01-01T00:00:00` even if this data is before the `from` date.

#### statistics

```graphql
type Market(?symbol: string, ? symbol_address: string){
		statistics: TMarketStats {
			volume: uint
		}
}
```

Provides a view of rolling 24 hour volume on the Flex for the market.

### User data

```graphql
type UserData(?pubkey: string, ?client_address: string): TUserData {
		equity: uint
		orders(?market): TOrder,
		trades(?market): TTrade,
		tokens(): TUserToken[]
	}
```

#### equity

Equity - total balance of all tokens converted to XXX currency price

```graphql
type UserData(?pubkey: string, ?client_address: string): TUserData {
		equity: uint  # 
	}
```

#### tokens

```graphql
type UserData(?pubkey: string, ?client_address: string): TUserData {
		tokens(): TUserToken[]
	}

type TUserToken {
	wallet_address: address
	token_symbol: string, # for example KWT
	token_root: address,
	balance: uint, 
	balance_in_orders: uint 
	decimals: uint
}
```

#### orders

```graphql
	type UserData(?pubkey: string, ?client_address: string): TUserData {
		orders(?market): TOrder,
	},
type TOrder{
	symbol: string
	amount: decimal
	price: decimal
	status: Enum {active, calceled, partially_executed, fully_executed}
	last_updated: datetime
}
```

#### trades

```graphql
	type UserData(?pubkey: string, ?client_address: string): TUserData {
		trades(?market): TOrder,
	},
type TOrder{
	symbol: string
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
		}
}
```

