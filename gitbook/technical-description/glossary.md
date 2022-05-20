# Glossary

## Flex Glossary

* **Super Root** - a contract that is used for Flex migrations. Stores information the Flex Contract System migrations and some additional statuses that reflect the exchange state.
* **Flex Contract System** - a set of contracts that altogether work as a decentralized exchange. Consists of 3 groups of contracts: Wallet (manages token wrappers and wallets), Exchange (manages pairs and orders) and User (supports user authorization and user funds management).
* **Global Config** - contract that stores all the data related to 1 Flex exchange migration ( wallet, exchange and user groups versions)
* **Flex Service** - general name for a set of Contract Indexers (services that parse blockchain Flex data) together with API service that provide read access to Flex data

## **User data Glossary**

* **DApp** - Desktop Flex Application
* **Trader** - person who trades on Flex from DApp.
* **Flex Client** - someone who registered on Flex via flex debot and has FlexClient contract that helps manage funds on Flex. Flex Client can delegate (lend) some of her money to Traders who can trade on Flex from DApp.
* **DApp login and pincode** - credentials, made up by Trader, pincode is used to encrypt DApp keys in the device’s storage (where desktop application is running)
* **DApp authorization** - process of passing DApp’s hash of pincode, login and Dapp public key to Surf via QR code and receiving Trader’s user\_id (it is calculated in Surf and placed on blockchain in a special User Index contract).
*   **DApp** **user\_id** - value that DApp receives in the end of authorization process. DApp will use it later to create orders and to search for Trader’s wallets. Identifies Trader’s device.

    ```graphql
    user_id = hash(surf_private_key, hash(DApp_pincode, DApp_login)).  
    ```
* **Trader’s wallet** - wallet that Flex Client deploys for Trader when delegating some funds. If Flex user trades alone, then Flex Client owner is the Trader herself.
* **DApp pubkey update** - temporary pubkey that is generated in DApp’s background for the user upon DApp installation, so it changes with each re-install, and when it is removed from application storage (by cleaning cache or after its lifetime has ended). Meanwhile user\_id stays constant, provided that user uses the same login+pincode. Every time the dapp is installed Authorization is performed and if user\_id already exists and pubkey was changed - Flex Debot updates pubkey in all wallets with lent tokens for this Trader.
* **Order** - request for sell or buy, created by user. Orders can be in one of the following states: not executed, partially executed, fully executed.

