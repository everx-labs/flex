# Limit Orders

> A **limit order** is an order to buy a security at no more than a specific price, or to sell a security at no less than a specific price (called "or better" for either direction). This gives the trader (customer) control over the price at which the trade is executed; however, the order may never be executed ("filled").

#### Limit Order of size V in quote tokens and price P in base tokens

```markdown
# Pseudocode for incoming limit order of size V and limit price P
Requirement: ability to find all deployed price contracts for the pair -> order_book
[LOOP] 
while V > 0 AND there are order_book.counter-orders with price P' equal or better than P:
	-select price contracts with price P' equal or better than P (e.g. P' <= P for incoming buy order)
  -find counter-order with best_price and size V' (e.g. min price for incoming buy order)
  -fill the incoming order for min(V',V)
  -update V = V - V'
[END OF LOOP]
if V > 0:
	place the incoming order with size V in OB
	wait for trade or cancel instruction
update balances in quote and base tokens
```

> **Maker orders:** These are orders that are not executed immediately and are added to the order book. Since maker orders increase the liquidity on the exchange, such orders are rewarded with negative trading fees. This means that traders receive a maker fee when a maker order gets executed.&#x20;
>
> **Taker orders:** These are orders that are matched immediately against an existing order. Since these orders reduce the liquidity on the exchange, they are required to pay positive trading fees. This means that traders pay taker fee when a taker order gets executed.

**POST-only orders** are ALWAYS executed as maker orders. This means:

1. Post-only orders always receive maker fee on execution;
2. Only limit orders can be post-only. Since market orders execute immediately, they can’t be made post-only;
3. If a post-only order will partially or fully match against an existing order in the order book, then the post-only order is cancelled.

#### Limit Order + POST Only option

```markdown
# Pseudocode
find counter-order with best_price
if best_price is equal or better than P:
    cancel order and notify the trader
else:
    place the order in OB
    wait for trade or cancel instruction
    update balances in quote and base tokens
```

#### How it is implemented on Flex:

Maker and Taker orders are flags.

Limit order will be implemented in Flex DeBot.
