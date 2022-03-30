# Glossary

* **DApp** - Desktop Flex Application (written in Electron)
* **Trader** - person who trades on Flex from DApp.
* **Flex Client** - someone who registered on Flex via flex debot and has FlexClient contract that helps manage funds on Flex. Flex Client can delegate (lend) some of her money to Traders who can trade on flex from DApp
* **DApp pincode** - pincode, made up by Trader, used to encrypt DApp keys in the device’s storage
* **DApp authorization** - process of passing DApp’s hash of pincode and public key to Surf via QR code and receiving Trader’s user\_id (it is calculated in Surf and placed on blockchain in a special contract which we do not care about here).
*   **DApp** **user\_id** - value that DApp receives in the end of authorization process. DApp will use it later to create orders and to search for Trader’s wallets. Identifies Trader’s device.

    ```graphql
    user_id = hash(surf_private_key, hash(DApp_pincode)).  
    ```
* **Trader’s wallet** - wallet that Flex Client deploys for Trader when delegating some funds.
* **Lent balance** - after Flex debot receives DApp pubkey and pincode and calculates user\_id, it can lend some portion of balance to the `user_id` for a period of time. It also has to map it with dapp pubkey, so the dapp can sign messages with its keys.
* **DApp pubkey update** - temporary pubkey that is generated in DApp’s background for the user upon DApp installation, so it changes with each re-install, and when it is removed from application storage (by cleaning cache or after its lifetime has ended). Meanwhile user\_id stays constant, provided that user uses the same pincode. Every time the dapp is installed Authorization is performed and if user\_id already exists and pubkey was changed - Flex Debot updates pubkey in all wallets with lent tokens for this user.
* **order** - request for sell or buy, created by user. Orders can be in one of the following states: not executed, partially executed, fully executed.
