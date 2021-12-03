# Market Orders

> A **market order** is a buy or sell order to be executed immediately at the _current_ _market_ prices. As long as there are willing sellers and buyers, market orders are filled. Market orders are used when certainty of execution is a priority over the price of execution. The order is filled at the best price available at the relevant time. In fast-moving markets, the price paid or received may be quite different from the last price quoted before the order was entered. A market order may be split across multiple participants on the other side of the transaction, resulting in different prices for some of the shares.

#### Market Order of size V in quote tokens

```markdown
# Pseudocode for incoming market order of size V
Requirement: ability to find all deployed price contracts for the pair -> order_book
[LOOP]
while V > 0 AND there are order_book.counter-orders:
	- select price contracts with counter-orders (e.g. getSellAmount > null) 
	- find counter-order with best price and size V' (e.g. min price for incoming buy order)
	- fill the incoming order for min(V',V)
  - update V = V - V'
[END OF LOOP]
update balances in quote and base tokens
```

#### How it is implemented on Flex:

DeBot places order at a specific price, if the price contract has counterparty at the same or above quantity it will be immediately executed. If counterparty has less than the order amount it will be returned or placed in the order book at the same price as a maker order.

###

###
