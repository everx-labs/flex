# Client App Authorization

As noted previously, convenience and complex trading use cases may call for a more comprehensive interface, than chat-based DeBots.

Flex thus supports the option to authorize a trading application to work with the user's TIP3 Wallet directly by lending ownership of it to an app the user trusts. This way Flex workflows may be streamlined, where they previously required constant [manual confirmation](../guides/how-to-trade-on-flex.md) of actions.

The app is identified by an external public key that the user has to specify.

The following diagram details authorization procedure.

![](../.gitbook/assets/Authorization\_Flex.png)

As TIP3 wallets are normally governed by the user's Flex Client contract, the authorization needs to be preformed by it as well, in a similar fashion to how ownership of the wallet can be lent to a Price exchange contract.

Ownership is always lent for a specified amount of tokens and time period.
