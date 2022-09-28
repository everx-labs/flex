# Time in force

> **Good-til-cancelled** (GTC) orders require a specific cancelling order and can persist indefinitely (or can be limited by say 90 days).&#x20;
>
> **Immediate or cancel** (IOC) orders are immediately executed or cancelled by the exchange. Unlike FOK orders, IOC orders allow for partial fills.&#x20;

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
