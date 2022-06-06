#pragma once

namespace tvm {

namespace immutable_ids {

/// IFlex::onDeploy func id
constexpr unsigned flex_on_deploy_id = 0x100;
/// IFlex::registerXchgPair func id
constexpr unsigned flex_register_xchg_pair_id = 0x200;
/// IFlex::requestPairs func id
constexpr unsigned flex_request_pairs_id = 0x300;

/// IWrapper::getDetails func id
constexpr unsigned wrapper_get_details_id = 0x100;
/// IWrapper::getTip3Config func id
constexpr unsigned wrapper_get_tip3_config_id = 0x200;
/// IWrapper::getWalletAddress func id
constexpr unsigned wrapper_get_wallet_address_id = 0x300;
/// IWrapper::cloned func id
constexpr unsigned wrapper_cloned_id = 0x400;
/// IWrapper::setCloned func id
constexpr unsigned wrapper_set_cloned_id = 0x500;

/// IWrapperDeployer::deploy func id
constexpr unsigned wrapper_deployer_deploy_id = 0x1111;

/// IXchgPair::getDetails func id
constexpr unsigned pair_get_details_id = 0x100;
/// IXchgPair::getPriceXchgCode func id
constexpr unsigned pair_get_price_xchg_code_id = 0x200;

/// ITONTokenWallet::getDetails func id
constexpr unsigned wallet_get_details_id = 0x100;

} // namespace immutable_ids

} // namespace tvm
