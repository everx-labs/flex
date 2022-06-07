# Trade on Flex

## What can I trade?

You can buy any listed tokens, or sell any listed tokens you own.

Use the drop-down Markets to select a token you want to trade:

![](<../.gitbook/assets/Screenshot from 2022-03-29 12-02-48.png>)

You will be able to deposit your funds into Flex to start trading any other tokens listed there.

## How to get started

To trade on Flex you need to [Connect your Surf wallet](connect-your-wallet.md) and deposit some funds. Once this is done, all your funds on Flex will be shown in the Wallets section.

![](../.gitbook/assets/015.png)

## How to deposit

Usually some funds are deposited during the [initial authorization](connect-your-wallet.md).

If you want to deposit more, or didn't deposit any after authorizing, click on the **Deposit button** next to the Asset you want to deposit, or on the **Surf** button in the upper right corner.

![](../.gitbook/assets/002.png)

Scan QR code or go to desktop Surf app.

The Flex Auth DeBot will launch. Select Deposit EVER or Deposit TIP-3.1, depending on the asset you want to deposit.

![](../.gitbook/assets/011.jpg)

Enter amount to be deposited.

![](../.gitbook/assets/008.jpg)

Enter amount to use for transaction fee (everything above the fee will be sent to the asset's Flex internal wallet and will be later used to pay for gas during transactions.

![](../.gitbook/assets/009.jpg)

Confirm transaction.

![](../.gitbook/assets/010.jpg)

Return to Flex. Now you can use the funds you deposited to trade.

## How to buy/sell

Flex displays all available information about a token which can help you decide on a trade. Graphs visualize token status and history, while the **Orderbook** and **Trades** tabs list the currently available orders and accumulated trade statistics.

![](../.gitbook/assets/22.png)

![](../.gitbook/assets/23.png)

Your current assets on Flex are always displayed in the Wallets section:

![](../.gitbook/assets/022.png)

{% hint style="info" %}
**Gas** balance is the native EVERs you use to pay transaction fees. It should be [kept above 50](keep-up-gas-balance.md) to make sure you can perform any actions on Flex.
{% endhint %}

Decide on a trade, and fill in your order in the leftmost section of the page:

![](../.gitbook/assets/017.png)

You can set your own price and amount, or click on an order in the **Orderbook** to fulfill it. Click the **BUY** or **SELL** button, once you're ready.&#x20;

If you set neither **POST** nor **IOC** flag:

* **When buying**: if the tokens you want to buy are available in the amount you're requesting at a price equal to the one you specified, Flex will facilitate the trade immediately for however many currently available tokens fit your conditions and create a buy order for the rest. If none are available, a buy order will be created for the whole amount.
* **When selling**: if there is a suitable buy order for the tokens you want to sell, or a part of it, the trade will be executed immediately, otherwise a buy order will be created for part or all of the tokens you are selling.

**POST** flag will place your order on the market to wait for someone else to fulfill it. You will become a market maker and benefit from Flex fees.

**IOC** flag creates and immediate-or-cancel order. If there are no orders on the market fitting yours, it will be cancelled. Otherwise, it will be immediately fulfilled.

A notification confirming your order creation will appear in the top right corner.

![](../.gitbook/assets/019.png)

While your order remains opened in the Orderbook, it is listed in your **Open orders** tab at the bottom of the page.

![](../.gitbook/assets/020.png)

Once your order is fulfilled, it appears in your **Trade history** tab at the bottom of the page.

![](../.gitbook/assets/021.png)

## How to withdraw

Go to the **Funds** section at the bottom and click Withdraw next to the asset you wish to withdraw.

![](../.gitbook/assets/023.png)

Scan QR code or go to desktop Surf app.

![](../.gitbook/assets/002.png)

The Flex auth DeBot will launch. Enter amount of tokens to unwrap (withdraw).

![](../.gitbook/assets/024.jpg)

Select the address which is going to be the owner of the tokens you are withdrawing.&#x20;

![](../.gitbook/assets/025.jpg)

Confirm transaction. It takes about 1.5 EVERs to perform.

![](../.gitbook/assets/026.jpg)

You can see your withdrawn tokens in [https://ever.live](https://ever.live) on the page your Surf wallet in the **Token balances** section.

![](../.gitbook/assets/027.png)
