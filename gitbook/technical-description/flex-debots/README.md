# Flex DeBots

Flex DeBots serve as a decentralized interface to Flex contracts. The main idea of using open-source DeBots is storing all business logic on-chain in an easily verifiable way, while providing simple dialogue-based interfaces for users to interact with.

DeBots work in any browser that supports the required features, such as [Surf](https://ton.surf) or [TONOS-CLI](https://github.com/tonlabs/tonos-cli). So long as the user trusts the browser, no third-party middleware is required to use them and be confident in the safety one's assets.

Sufficiently complex systems of [DeBots](https://github.com/tonlabs/debots) can implement any required trading logic.

However, if needed, DeBots may be integrated into other software as well.

View Flex DeBot source code [here](https://github.com/tonlabs/flex/tree/main/debots).

## Flex DeBots Overview <a href="#_fx2fa8m3626n" id="_fx2fa8m3626n"></a>

### Flex DeBot <a href="#_fx2fa8m3626n" id="_fx2fa8m3626n"></a>

[Flex DeBot](./#\_fx2fa8m3626n-1) helps buy and sell TIP3 tokens:

* queue all available pairs
* queue all price contracts in the pair and display current sell/buy order quantity
* show current book price
* create and send Good-til-cancelled (GTC) order for a selected token quantity for a selected Price. Since this will require to have a token wallet and to transfer the ownership of the user wallet to the Price Contract it should do all this just by asking user for a seed phrase or private key once
* display active user orders
* withdraw active user orders

### TIP3 DeBot <a href="#_643rfn3xmp76" id="_643rfn3xmp76"></a>

[TIP3 DeBot](tip3debot.md) helps to manage TIP3 tokens:

* create a new TIP3 token,
* grant TIP3 tokens to TIP3 wallets.

### AMM DeBot <a href="#_40zf09lfhgau" id="_40zf09lfhgau"></a>

[AMM DeBot](https://github.com/tonlabs/flex/tree/main/debots/AMM/MMDebot) helps to set up and configure a new Automated Market Maker for the Flex exchange.

### Liquidity DeBot <a href="#_7px6az888oc5" id="_7px6az888oc5"></a>

[Liquidity DeBot](https://github.com/tonlabs/flex/tree/main/debots/AMM/PoolDebot) helps users to provide and withdraw liquidity to AMM Liquidity pools.
