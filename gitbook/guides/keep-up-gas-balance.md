# Keep up Gas Balance

Flex works entirely on-chain as a set of smart contracts. This means that every action you take requires [blockchain fees](../specifications/flex-fees.md) to be paid.

To cover these fees Flex stores an amount of native EVER tokens as your **Gas balance**.

You allocate the initial amount to your Gas balance when connecting your wallet. After deployment it will be at a minimum of about 130 EVERs.

This balance will gradually be spent on deploying new internal wallets (\~10 EVERs per wallet) and topping them up as necessary, as they spend their balances on trades.

It is recommended to keep your Gas balance up above 50 EVERs at all times.

Its currently available value is always displayed in the top right corner of te Flex window.

<figure><img src="../.gitbook/assets/074.png" alt=""><figcaption></figcaption></figure>

If it turns <mark style="color:red;">red</mark>, it needs to be topped up.

To top it up, click on the displayed **Gas** balance.

Scan the QR code or go to desktop Surf app.

<figure><img src="../.gitbook/assets/0044.png" alt=""><figcaption></figcaption></figure>

Enter the amount to send to your Gas balance and confirm:

<figure><img src="../.gitbook/assets/0030.png" alt=""><figcaption></figcaption></figure>
