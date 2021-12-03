# Time in force

> A **day order** or **good for day order** (GFD) (the most common) is a market or limit order that is in force from the time the order is submitted to the end of the day's trading session.&#x20;
>
> **Good-til-cancelled** (GTC) orders require a specific cancelling order and can persist indefinitely (or can be limited by say 90 days).&#x20;
>
> **Immediate or cancel** (IOC) orders are immediately executed or cancelled by the exchange. Unlike FOK orders, IOC orders allow for partial fills.&#x20;
>
> **Fill or kill** (FOK) orders are usually limit orders that must be executed or cancelled immediately. Unlike IOC orders, FOK orders require the full quantity to be executed.

#### Limit Order + IOC option

```markdown
# Pseudocode
while order_filled < V and there are counter-orders:
    find counter-order with best_price
    if best_price is equal or better than P:
        fill the order
    else:
				break
update balances in quote and base tokens
```

#### How it is implemented on Flex:

Price contract needs to support these types of execution flags.
