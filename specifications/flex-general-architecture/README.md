# FLEX General Architecture

## CLOB implementation in Flex

Flex achieves load balancing and speed of execution by distributing the logic among many atomic smart contracts, while simultaneously transferring search and decision logic to user computers. All Order Book contracts addresses are deterministically calculated: the Trading Pair and its Price order contracts.

> Flex trades exclusively in Flex TIP3 tokens. External [TIP3](https://github.com/tonlabs/ton-labs-contracts/tree/master/cpp) tokens or any other type of tokens, including native EVERs are introduced to Flex via a Flex TIP3 [wrapper](../../technical-description/overview.md#wrapper-tip3-root) for the token.

\
For instance, XTIP3-YTIP3 will resolve a Root of XTIP3-YTIP3 trading pair. Users then can calculate an address of a particular Price Contract by entering a Price. “Code Hash=$XTIP3-YTIP3 Data=1.2” will calculate an address of a contract that currently trades a XTIP3-YTIP3 pair at 1.2 XTIP3 per 1 YTIP3 token.

After performing the calculation of addresses, the user performs a read operation: querying the blockchain database with [SDK](https://tonlabs.gitbook.io/ton-sdk/) over GraphQL. Since each user only calculates pairs and prices they need, the operation takes microseconds and one query is almost instantaneous.

Trading step value for a pair will be provided in the pair's Root Contract data. Users can now calculate a whole order book of this pair by entering all price steps around the target price or ask the database for all addresses of that Price contract code hash. After calculating all price step contract addresses, a user may try to retrieve all these contracts. If the contract does not exist, it means there are no orders of that price.

![](../../.gitbook/assets/flex.svg)



####
