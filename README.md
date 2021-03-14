# Flash Exchange (FLEX)

## TIP3 DeBot

[`tipe3Debot`](https://github.com/tonlabs/flex/blob/main/tip3Debot/tip3Debot.sol) - DeBot that helps manage TIP3 tokens: create new TIP3 token, grant TIP3 tokens to TIP3 wallets.

Compile DeBot using [`tondev`](https://github.com/tonlabs/tondev) tool:

    tondev sol compile tip3Debot.sol

### Try it

DeBot is already deployed to mainnet and devnet. Try it using [`tonos-cli`](https://github.com/tonlabs/tonos-cli):

    tonos-cli debot --url <network> debot fetch 0:81c12c2f4514124536aafea59db7df0262d3af877b4477afe6514bbc5bc9f317

<network> - `net.ton.dev` or `main.ton.dev`.
