/** \file
 *  \brief SuperRootOwner (single owner) contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "SuperRootOwner.hpp"
#include "SuperRoot.hpp"
#include "FlexSalt.hpp"
#include "WrappersConfig.hpp"
#include "Flex.hpp"
#include "UserDataConfig.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// SuperRoot contract implementation
class SuperRootOwner final : public smart_interface<ISuperRootOwner>, public DSuperRootOwner {
  using data = DSuperRootOwner;

public:
  static constexpr unsigned TIMESTAMP_DELAY = 1800;
  using replay_protection_t = replay_attack_protection::timestamp<TIMESTAMP_DELAY>;

  DEFAULT_SUPPORT_FUNCTIONS(ISuperRootOwner, replay_protection_t)

  static constexpr bool cant_work_during_update = true;
  static constexpr bool allowed_during_update = false;
  static constexpr bool starting_update = true;
  static constexpr bool not_starting_update = false;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned uninitialized                  = 101; ///< Uninitialized
    static constexpr unsigned super_root_not_deployed        = 102; ///< SuperRoot not deployed
  };

  void constructor(
    uint256 pubkey
  ) {
    tvm_accept();
    pubkey_ = pubkey;
  }

  void setCode(uint8 type, cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    switch (static_cast<sroot_code_type>(type.get())) {
    case sroot_code_type::super_root: setSuperRootCode(code); return;
    case sroot_code_type::global_cfg: setGlobalConfigCode(code); return;
    case sroot_code_type::flex_client_stub: setFlexClientStubCode(code); return;
    case sroot_code_type::wrappers_cfg: setWrappersConfigCode(code); return;
    case sroot_code_type::wic: setWICCode(code); return;
    case sroot_code_type::flex: setFlexCode(code); return;
    case sroot_code_type::pair: setPairCode(code); return;
    case sroot_code_type::price: setPriceCode(code); return;
    case sroot_code_type::user_data_cfg: setUserDataConfigCode(code); return;
    case sroot_code_type::flex_client: setFlexClientCode(code); return;
    case sroot_code_type::auth_index: setAuthIndexCode(code); return;
    case sroot_code_type::user_id_index: setUserIdIndexCode(code); return;
    }
  }

  void setSuperRootCode(cell code) {
    super_root_code_ = code;
  }

  void setGlobalConfigCode(cell code) {
    global_cfg_code_ = code;
  }

  void setFlexClientStubCode(cell code) {
    flex_client_stub_ = code;
  }

  void setWrappersConfigCode(cell code) {
    wrappers_cfg_code_ = code;
  }

  void setWICCode(cell code) {
    wic_code_ = code;
  }

  void setFlexCode(cell code) {
    flex_code_ = code;
  }

  void setPairCode(cell code) {
    pair_code_ = code;
  }

  void setPriceCode(cell code) {
    price_code_ = code;
  }

  void setUserDataConfigCode(cell code) {
    user_data_cfg_code_ = code;
  }

  void setFlexClientCode(cell code) {
    flex_client_code_ = code;
  }

  void setAuthIndexCode(cell code) {
    auth_index_code_ = code;
  }

  void setUserIdIndexCode(cell code) {
    user_id_index_code_ = code;
  }

  address deploySuperRoot(
    uint128     evers,
    address_opt prev_super_root
  ) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(is_initialized(), error_code::uninitialized);
    tvm_accept();
    tvm_commit();

    auto [init, hash] = prepare<ISuperRoot>(DSuperRoot{.pubkey_ = pubkey_, .owner_ = tvm_myaddr()}, super_root_code_.get());
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    ISuperRootPtr ptr(address::make_std(workchain_id, hash));
    ptr.deploy(init, Evers(evers.get())).
      onDeploy(global_cfg_code_.get(), flex_client_stub_.get(), prev_super_root);
    super_root_ = ptr.get();
    return ptr.get();
  }

  auto impl(uint128 evers) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(is_initialized(), error_code::uninitialized);
    require(!!super_root_, error_code::super_root_not_deployed);
    tvm_accept();
    tvm_commit();
    return ISuperRootPtr(*super_root_)(Evers(evers.get()));
  }

  void update(
    uint128     main_evers,
    uint128     cfg_deploy_evers,
    uint128     cfg_keep_evers,
    FlexVersion version,
    address     wrappers_cfg,
    address     flex,
    address     user_cfg,
    string      description
  ) {
    impl(main_evers).update(cfg_deploy_evers, cfg_keep_evers, version, wrappers_cfg, flex, user_cfg, description);
  }

  void release(uint128 main_evers) {
    impl(main_evers).release();
  }

  void addWrapperType(
    uint128 main_evers,
    uint128 wrappers_cfg_keep_evers,
    address wrappers_cfg,
    uint8   type,
    address wrapper_deployer
  ) {
    auto sroot = impl(main_evers);
    cell msg = IWrappersConfigPtr(wrappers_cfg).prepare_internal(0_ev).
      addWrapperType(wrappers_cfg_keep_evers, type, wrapper_deployer);
    sroot.proxy(msg, cant_work_during_update, not_starting_update);
  }

  void addWrapper(
    uint128       main_evers,
    uint128       wrappers_cfg_keep_evers,
    address       wrappers_cfg,
    WICCloneEvers evers,
    string        symbol,
    uint8         type,
    cell          init_args
  ) {
    auto sroot = impl(main_evers);
    cell msg = IWrappersConfigPtr(wrappers_cfg).prepare_internal(0_ev).
      addWrapper(wrappers_cfg_keep_evers, evers, symbol, type, init_args);
    sroot.proxy(msg, cant_work_during_update, not_starting_update);
  }

  void addXchgPair(
    uint128        main_evers,
    address        flex,
    PairCloneEvers evers,
    Tip3Config     major_tip3cfg,
    Tip3Config     minor_tip3cfg,
    uint128        min_amount,
    uint128        minmove,
    uint128        price_denum,
    address        notify_addr
  ) {
    auto sroot = impl(main_evers);
    cell msg = IFlexPtr(flex).prepare_internal(0_ev).
      addXchgPair(evers, major_tip3cfg, minor_tip3cfg,
                  min_amount, minmove, price_denum, notify_addr);
    sroot.proxy(msg, cant_work_during_update, not_starting_update);
  }

  void unlistWrapper(
    uint128 main_evers,
    address wrappers_cfg,
    address wic
  ) {
    auto sroot = impl(main_evers);
    cell msg = IWrappersConfigPtr(wrappers_cfg).prepare_internal(0_ev).
      unlistWrapper(wic);
    sroot.proxy(msg, cant_work_during_update, not_starting_update);
  }

  void unlistXchgPair(
    uint128 main_evers,
    address flex,
    address pair
  ) {
    auto sroot = impl(main_evers);
    cell msg = IFlexPtr(flex).prepare_internal(0_ev).
      unlistXchgPair(pair);
    sroot.proxy(msg, cant_work_during_update, not_starting_update);
  }

  resumable<address> deployWrappersConfig(
    uint128 main_evers,
    uint128 deploy_evers,
    uint128 wrappers_cfg_keep_evers,
    uint32  token_version
  ) {
    co_return co_await impl(main_evers).
      deployWrappersConfig(deploy_evers, wrappers_cfg_keep_evers, token_version, wrappers_cfg_code_.get(), wic_code_.get());
  }

  resumable<address> deployFlex(
    uint128        main_evers,
    uint128        deploy_evers,
    uint128        keep_evers,
    PairCloneEvers evers,
    address_opt    old_flex,
    uint32         exchange_version,
    EversConfig    ev_cfg,
    uint8          deals_limit
  ) {
    require(flex_code_ && pair_code_ && price_code_, error_code::uninitialized);
    co_return co_await impl(main_evers).
      deployFlex(deploy_evers, keep_evers, evers, old_flex, exchange_version,
                 flex_code_.get(), pair_code_.get(), price_code_.get(), ev_cfg, deals_limit);
  }

  resumable<address> deployUserDataConfig(
    uint128     main_evers,
    uint128     deploy_evers,
    FlexVersion triplet,
    address     flex
  ) {
    require(user_data_cfg_code_ && flex_client_code_ && auth_index_code_ && user_id_index_code_ && price_code_,
            error_code::uninitialized);
    uint256 unsalted_price_code_hash(tvm_hash(price_code_.get()));
    bind_info binding { flex, unsalted_price_code_hash };
    co_return co_await impl(main_evers).
      deployUserDataConfig(deploy_evers, triplet, binding, user_data_cfg_code_.get(), flex_client_code_.get(),
                           auth_index_code_.get(), user_id_index_code_.get());
  }

  resumable<address> cloneWrappersConfig(
    uint128             main_evers,
    address             wrappers_cfg,
    uint128             wrapper_cfg_keep_evers,
    uint128             clone_deploy_evers,
    WICCloneEvers       wic_evers,
    uint32              new_token_version,
    dict_array<address> wrapper_deployers
  ) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(super_root_ && wrappers_cfg_code_ && wic_code_, error_code::uninitialized);
    tvm_accept();
    tvm_commit();
    ISuperRootPtr ptr(*super_root_);
    address cloned_wrappers_cfg = co_await ptr.tail_call<address>(wrappers_cfg, Evers(main_evers.get())).
      cloneWrappersConfig(
        wrappers_cfg, wrapper_cfg_keep_evers, clone_deploy_evers, wic_evers, new_token_version, wrapper_deployers
        );
    co_return cloned_wrappers_cfg;
  }

  void setFlags(
    uint128   main_evers,
    opt<bool> stop_trade,
    opt<bool> abandon_ship,
    opt<bool> update_started
  ) {
    impl(main_evers).setFlags(stop_trade, abandon_ship, update_started);
  }

  void transfer(
    uint128 main_evers,
    address to,
    uint128 evers
  ) {
    impl(main_evers).transfer(to, evers);
  }

  void transferReserveTokens(
    uint128 main_evers,
    address wrapper,
    uint128 tokens,
    address to,
    uint128 evers
  ) {
    impl(main_evers).transferReserveTokens(wrapper, tokens, to, evers);
  }

  void setOwner(
    uint128 main_evers,
    address owner
  ) {
    impl(main_evers).setOwner(owner);
  }

  void setUpdateTeam(
    uint128     main_evers,
    address_opt team
  ) {
    impl(main_evers).setUpdateTeam(team);
  }

  void setNextSuperRoot(
    uint128 main_evers,
    address next_super_root
  ) {
    impl(main_evers).setNextSuperRoot(next_super_root);
  }

  SuperRootOwnerDetails getDetails() {
    return {
      is_initialized(), pubkey_, super_root_,
      super_root_code_, global_cfg_code_, flex_client_stub_, wrappers_cfg_code_, wic_code_,
      flex_code_, pair_code_, price_code_,
      user_data_cfg_code_, flex_client_code_, auth_index_code_, user_id_index_code_
    };
  }

  bool is_initialized() const {
    return super_root_code_ && global_cfg_code_ && flex_client_stub_ && wrappers_cfg_code_ && wic_code_ &&
      flex_code_ && pair_code_ && price_code_ &&
      user_data_cfg_code_ && flex_client_code_ && auth_index_code_ && user_id_index_code_;
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  static int _on_bounced([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    tvm_accept();
    return 0;
  }
};

DEFINE_JSON_ABI(ISuperRootOwner, DSuperRootOwner, ESuperRootOwner, SuperRootOwner::replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(SuperRootOwner, ISuperRootOwner, DSuperRootOwner, SuperRootOwner::TIMESTAMP_DELAY)
