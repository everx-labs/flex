/** \file
 *  \brief SuperRoot contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "SuperRoot.hpp"
#include "GlobalConfig.hpp"
#include "WrappersConfig.hpp"
#include "UserDataConfig.hpp"
#include "FlexClientStub.hpp"
#include "FlexSalt.hpp"
#include "Flex.hpp"
#include "Wrapper.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

constexpr unsigned SROOT_REVISION = 1;

using namespace tvm;

/// Authentification configuration flags
struct auth_cfg {
  bool allowed_for_update_team; ///< If this method is allowed for the update team
};

/// SuperRoot contract implementation
class SuperRoot final : public smart_interface<ISuperRoot>, public DSuperRoot {
  using data = DSuperRoot;

public:
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned unexpected_confirmation        = 101; ///< Unexpected confirmation
    static constexpr unsigned version_unset                  = 102; ///< Current version is not set
    static constexpr unsigned beta_version_unset             = 103; ///< Beta version is not set
    static constexpr unsigned double_initialization          = 104; ///< Double initialization
    static constexpr unsigned uninitialized                  = 105; ///< Uninitialized
    static constexpr unsigned not_allowed_during_update      = 106; ///< Not allowed during update
  };

  void onDeploy(
    cell        global_config_code,
    cell        flex_client_stub,
    address_opt prev_super_root
  ) {
    require(int_sender() == owner_, error_code::message_sender_is_not_my_owner);
    require(!global_config_code_, error_code::double_initialization);

    workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    global_config_code_ = tvm_add_code_salt<GlobalConfigSalt>({.super_root = tvm_myaddr()}, global_config_code);
    flex_client_stub_ = tvm_add_code_salt<FlexClientStubSalt>({.super_root = tvm_myaddr()}, flex_client_stub);
    prev_super_root_ = prev_super_root;
    revision_ = uint32(SROOT_REVISION);
  }

  void update(
    uint128     cfg_deploy_evers,
    uint128     cfg_keep_evers,
    FlexVersion version,
    address     wrappers_cfg,
    address     flex,
    address     user_cfg,
    string      description
  ) {
    require(global_config_code_, error_code::uninitialized);
    check_owner({ .allowed_for_update_team = true });

    auto [init, hash] = prepare<IGlobalConfig>(DGlobalConfig{ .version_ = version }, global_config_code_.get());
    IGlobalConfigPtr ptr(address::make_std(workchain_id_, hash));
    ptr.deploy(init, Evers(cfg_deploy_evers.get())).onDeploy(cfg_keep_evers, wrappers_cfg, flex, user_cfg, description);
    deploying_cfg_ = ptr.get();
  }

  void updateConfirmed(FlexVersion version) {
    require(deploying_cfg_ && *deploying_cfg_ == int_sender(), error_code::unexpected_confirmation);
    beta_version_ = version;
    deploying_cfg_.reset();
  }

  void release() {
    require(!!beta_version_, error_code::beta_version_unset);
    check_owner({ .allowed_for_update_team = false });

    version_ = beta_version_;
    beta_version_.reset();
    update_started_ = false;
  }

  void proxy(
    cell msg,
    bool cant_work_during_update,
    bool starting_update
  ) {
    require(!cant_work_during_update || !update_started_.get() || !version_, error_code::not_allowed_during_update);
    if (starting_update)
      update_started_ = true;
    check_owner({ .allowed_for_update_team = false });
    tvm_sendmsg(msg, SEND_ALL_GAS);
  }

  void addWrapperType(
    uint128 call_evers,
    uint128 wrappers_cfg_keep_evers,
    address wrappers_cfg,
    uint8   type,
    address wrapper_deployer
  ) {
    require(!update_started_.get() || !version_, error_code::not_allowed_during_update);
    check_owner({ .allowed_for_update_team = false });
    IWrappersConfigPtr(wrappers_cfg)(Evers(call_evers.get())).addWrapperType(wrappers_cfg_keep_evers, type, wrapper_deployer);
  }

  void addWrapper(
    uint128       call_evers,
    uint128       wrappers_cfg_keep_evers,
    address       wrappers_cfg,
    WICCloneEvers evers,
    string        symbol,
    uint8         type,
    cell          init_args
  ) {
    require(!update_started_.get() || !version_, error_code::not_allowed_during_update);
    check_owner({ .allowed_for_update_team = false });
    IWrappersConfigPtr(wrappers_cfg)(Evers(call_evers.get())).addWrapper(wrappers_cfg_keep_evers, evers, symbol, type, init_args);
  }

  void addXchgPair(
    uint128        call_evers,
    address        flex,
    PairCloneEvers evers,
    Tip3Config     major_tip3cfg,
    Tip3Config     minor_tip3cfg,
    uint128        min_amount,
    uint128        minmove,
    uint128        price_denum,
    address        notify_addr
  ) {
    require(!update_started_.get() || !version_, error_code::not_allowed_during_update);
    check_owner({ .allowed_for_update_team = false });
    IFlexPtr(flex)(Evers(call_evers.get())).
      addXchgPair(evers, major_tip3cfg, minor_tip3cfg, min_amount, minmove, price_denum, notify_addr);
  }

  address deployWrappersConfig(
    uint128 deploy_evers,
    uint128 wrappers_cfg_keep_evers,
    uint32  token_version,
    cell    wrappers_cfg_code,
    cell    wic_code
  ) {
    check_owner({ .allowed_for_update_team = true });
    cell salted_code = tvm_add_code_salt<WrappersConfigSalt>({tvm_myaddr(), wic_code}, wrappers_cfg_code);

    auto [init, hash] = prepare<IWrappersConfig>(DWrappersConfig{.token_version_ = token_version}, salted_code);
    IWrappersConfigPtr ptr(address::make_std(workchain_id_, hash));
    ptr.deploy(init, Evers(deploy_evers.get())).onDeploy(wrappers_cfg_keep_evers, {}, {}, {}, {}, {}, 0u32);
    update_started_ = true;

    return ptr.get();
  }

  address deployFlex(
    uint128        deploy_evers,
    uint128        keep_evers,
    PairCloneEvers evers,
    address_opt    old_flex,
    uint32         exchange_version,
    cell           flex_code,
    cell           xchg_pair_code,
    cell           xchg_price_code,
    EversConfig    ev_cfg,
    uint8          deals_limit
  ) {
    check_owner({ .allowed_for_update_team = true });
    FlexSalt salt {
      .super_root      = tvm_myaddr(),
      .ev_cfg          = ev_cfg,
      .deals_limit     = deals_limit,
      .xchg_pair_code  = xchg_pair_code,
      .xchg_price_code = xchg_price_code
    };
    cell salted_code = tvm_add_code_salt<FlexSalt>(salt, flex_code);
    auto [init, hash] = prepare<IFlex>(DFlex{.exchange_version_ = exchange_version}, salted_code);
    IFlexPtr ptr(address::make_std(workchain_id_, hash));
    ptr.deploy(init, Evers(deploy_evers.get())).
      onDeploy(keep_evers, evers, old_flex);
    update_started_ = true;

    return ptr.get();
  }

  address deployUserDataConfig(
    uint128     deploy_evers,
    FlexVersion triplet,
    bind_info   binding,
    cell        user_data_cfg_code,
    cell        flex_client_code,
    cell        auth_index_code,
    cell        user_id_index_code
  ) {
    check_owner({ .allowed_for_update_team = true });
    cell salted_code = tvm_add_code_salt<UserDataConfigSalt>({tvm_myaddr()}, user_data_cfg_code);

    auto [init, hash] = prepare<IUserDataConfig>(DUserDataConfig{.triplet_ = triplet}, salted_code);
    IUserDataConfigPtr ptr(address::make_std(workchain_id_, hash));
    ptr.deploy(init, Evers(deploy_evers.get())).
      onDeploy(binding, flex_client_stub_.get(), flex_client_code, auth_index_code, user_id_index_code);
    update_started_ = true;

    return ptr.get();
  }

  void cloneWrappersConfig(
    address             wrappers_cfg,
    uint128             wrapper_cfg_keep_evers,
    uint128             clone_deploy_evers,
    WICCloneEvers       wic_evers,
    uint32              new_token_version,
    dict_array<address> wrapper_deployers
  ) {
    check_owner({ .allowed_for_update_team = true });

    update_started_ = true;

    IWrappersConfigPtr ptr(wrappers_cfg);
    // performing `tail call` - requesting dest to answer to our caller
    temporary_data::setglob(global_id::answer_id, return_func_id()->get());
    ptr(0_ev, SEND_ALL_GAS).
      cloneUpgrade(int_sender(), wrapper_cfg_keep_evers, clone_deploy_evers, wic_evers, new_token_version, wrapper_deployers);
  }

  void transfer(
    address to,
    uint128 evers
  ) {
    check_owner<true>({ .allowed_for_update_team = false });
    tvm_transfer(to, evers.get(), true);
  }

  void transferReserveTokens(
    address wrapper,
    uint128 tokens,
    address to
  ) {
    check_owner({ .allowed_for_update_team = false });
    IWrapperPtr(wrapper)(0_ev, SEND_ALL_GAS).transferFromReserveWallet(int_sender(), to, tokens);
  }

  void setFlags(
    opt<bool> stop_trade,
    opt<bool> abandon_ship,
    opt<bool> update_started
  ) {
    check_owner({ .allowed_for_update_team = false });
    if (stop_trade) stop_trade_ = *stop_trade;
    if (abandon_ship) abandon_ship_ = *abandon_ship;
    if (update_started) update_started_ = *update_started;
  }

  void setOwner(
    address owner
  ) {
    check_owner({ .allowed_for_update_team = false });
    owner_ = owner;
  }

  void setUpdateTeam(
    address_opt team
  ) {
    check_owner({ .allowed_for_update_team = false });
    update_team_ = team;
  }

  void setNextSuperRoot(
    address next_super_root
  ) {
    check_owner({ .allowed_for_update_team = false });
    next_super_root_ = next_super_root;
  }

  SuperRootDetails getDetails() {
    require(global_config_code_, error_code::uninitialized);
    address_opt cur_cfg, beta_cfg;
    if (version_)
      cur_cfg = getGlobalConfig(*version_);
    if (beta_version_)
      beta_cfg = getGlobalConfig(*beta_version_);
    return {
      pubkey_, stop_trade_.get(), abandon_ship_.get(), update_started_.get(), owner_, update_team_, global_config_code_.get(),
      uint256(tvm_hash(global_config_code_.get())), workchain_id_, version_, beta_version_, deploying_cfg_, cur_cfg, beta_cfg,
      prev_super_root_, next_super_root_, revision_
    };
  }

  address getGlobalConfig(FlexVersion version) {
    require(global_config_code_, error_code::uninitialized);
    [[maybe_unused]] auto [init, std_addr] = prepare<IGlobalConfig>(DGlobalConfig{ .version_ = version }, global_config_code_.get());
    return address::make_std(workchain_id_, std_addr);
  }

  address getCurrentGlobalConfig() {
    require(!!version_, error_code::version_unset);
    return getGlobalConfig(*version_);
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }

  // `out_transfer = true` allows to transfer stored evers outside
  template<bool out_transfer = false>
  void check_owner(auth_cfg cfg) {
    bool update_team_auth = cfg.allowed_for_update_team && update_team_ && (int_sender() == *update_team_);
    bool owner_auth = (int_sender() == owner_);
    require(update_team_auth || owner_auth, error_code::message_sender_is_not_my_owner);
    if constexpr (!out_transfer) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    if (!update_team_auth)
      tvm_accept();
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(ISuperRoot, void)
};

DEFINE_JSON_ABI(ISuperRoot, DSuperRoot, ESuperRoot);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(SuperRoot, ISuperRoot, DSuperRoot)
