pragma ton-solidity >=0.47.0;

import "../structures/EversConfig.sol";
import "../structures/ListingConfig.sol";
import "../structures/FlexOwnershipInfo.sol";
import "../structures/WrapperListingRequest.sol";
import "../structures/XchgPairListingRequest.sol";

struct PriceTuple {
    uint128 num;
    uint128 denum;
}

abstract contract AFlex {
    function getDetails() public functionID(0x17) returns(
        bool initialized,
        EversConfig ev_cfg,
        ListingConfig listing_cfg,
        TvmCell xchg_pair_code,
        TvmCell wrapper_code,
        TvmCell wrapper_ever_code,
        uint8 deals_limit,
        uint256 unsalted_price_code_hash,
        FlexOwnershipInfo ownership,
        WrapperListingRequestWithPubkey[] wrapper_listing_requests,
        XchgPairListingRequestWithPubkey[] xchg_pair_listing_requests
    ) {}
    function calcLendTokensForOrder(bool sell,uint128 major_tokens, PriceTuple price) public functionID(0x1a) returns(uint128 value0) {}

}