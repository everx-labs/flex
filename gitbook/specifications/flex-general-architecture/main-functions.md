# Main Functions

### Ability to create new pairs

Flex works with TIP3/TIP3 pairs. New token pairs can be added to Flex by deploying a new Pair Root contract from the Exchange Root contract. It can be done with the help of the Listing DeBot. Non-TIP3 tokens can be added via a TIP3 wrapper.

### Ability to create new orders

If the price contract does not exist, a User can create a "Good-til-cancelled (GTC) order" by deploying the pair contract adding the price as a contract data in the constructor and sending the required amount of money to cover the trade.

For example, passing a price of $0.55 YTIP3 into the constructor and sending 10,000 XTIP3 (major token) to the contract will create a sell order of 10,000 XTIP3 for the price of 0.55 YTIP3 per 1 XTIP3 in a XTIP3-YTIP3 trading pair contract. Sending YTIP3 tokens will create a buy order.

A User can Cancel un-executed orders at any time. Flex orders also have expiration time set by users to avoid a long-forgotten order suddenly being filled.

### Exchange tokens

All matched orders in the contract are executed immediately. If the contract does not have any orders it will terminate itself. This is important as the existence of the price contract at the calculated address by itself means orders of that price exist.

If an order "Good till cancelled" cannot be executed in one transaction it should execute itself in several sending each time a message to itself.

Specialized exchange orders allowing for more complex trading strategies will also be supported:

* [Market Orders](market-orders.md)
* [Limit Orders](limit-orders.md)
* [Time in force](time-in-force.md)

### Composition of the Order Book

Just by retrieving all deployed contracts by a contract code hash, a user can see the whole stack of the order book. Flex Order Books for token pairs are available in the standalone decentralized interface.

### Trade history

User client can retrieve all messages for a trading pair contract and sort them by time to find the latest trades of that pair. Successful trade transactions should emit a message so it could be found later. The trade history is available through the Flex decentralized interface.
