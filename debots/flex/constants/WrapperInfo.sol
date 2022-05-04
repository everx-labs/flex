pragma ton-solidity >=0.47.0;

library WrapperInfo {
    uint256 constant TIP3_WRAPPER_CH = 0x370d08078a00cbc699fc07d21b313e602e56c7b4bf65588a6179eb897ad47c21;
    uint256 constant EVER_WRAPPER_CH = 0x714b364ea4b912b7396358616210ddd65cb36213eb9a66a68de613abed90a72c;
    uint256 constant UNSALTED_PRICE_CH = 0xc1047036e3bd2ca48e2d641b697475e19e5c37e0c9244eda9424b750f871132c;
}

abstract contract EverWrapperAddr{
    address constant EVER_WRAPPER = address(0x9e82b1e79ae4d3fc6aecab8a323bf1ed829b450b909721db6524c04193d9d6d3);
}
