# Keep up Gas Balance

Flex works entirely on-chain as a set of smart contracts. This means that every action you take requires [blockchain fees](../specifications/flex-fees.md) to be paid.

To cover these fees Flex stores an amount of native EVER tokens as your **Gas balance**.

You allocate the initial amount to your Gas balance when [connecting](connect-your-wallet.md) your wallet.&#x20;

This balance will gradually be spent on trading and deploying new internal wallets whenever you acquire an asset you did not previously own (\~10 EVERs per new asset wallet).

Its currently available value is always displayed in the top right corner of the Flex window.

<figure><img src="../.gitbook/assets/074.png" alt=""><figcaption></figcaption></figure>

**Note**: The balance displays your total gas on Flex. Of this amount 20 EVERs are reserved on your Flex Client contract for various system actions. The rest is used to pay for gas on the exchange.&#x20;

If the balance turns <mark style="color:red;">red</mark>, it needs to be topped up.

To top it up, click on the displayed **Gas** balance.

Scan the QR code or go to desktop Surf app.

<figure><img src="../.gitbook/assets/0044.png" alt=""><figcaption></figcaption></figure>

Enter the amount to send to your Gas balance and confirm:

<figure><img src="../.gitbook/assets/0030.png" alt=""><figcaption></figcaption></figure>
