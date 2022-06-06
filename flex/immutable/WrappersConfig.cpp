/** \file
 *  \brief WrappersConfig contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "WrappersConfig.hpp"
#include "WIC.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// WrappersConfig contract implementation
class WrappersConfig final : public smart_interface<IWrappersConfig>, public DWrappersConfig {
  using data = DWrappersConfig;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IWrappersConfig, void)
  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner          = 100; ///< Authorization error
    static constexpr unsigned only_super_root_may_deploy_me           = 101; ///< Only SuperRoot contract may deploy this contract
    static constexpr unsigned wrong_token_version                     = 102; ///< Wrong token version
    static constexpr unsigned wrong_deployers_size                    = 103; ///< Wrong Wrapper deployers size
    static constexpr unsigned not_correct_previous_clone_deploying_me = 104; ///< Not a correct previous clone deploying me
    static constexpr unsigned unexpected_callback                     = 105; ///< Unexpected callback
    static constexpr unsigned not_enough_evers                        = 106; ///< Not enough evers for processing
    static constexpr unsigned wrong_wrapper_type_num                  = 107; ///< Wrong wrapper type number
    static constexpr unsigned wrong_initialization                    = 108; ///< Wrong initialization
    static constexpr unsigned missed_evers_cfg                        = 109; ///< Missed evers configuration
  };

  void onDeploy(
    uint128             keep_evers,
    opt<WICCloneEvers>  evers_opt,
    opt<uint32>         old_token_version,
    dict_array<address> wrapper_deployers,
    address_opt         first_wic,
    address_opt         last_wic,
    uint32              wic_count
  ) {
    require(!deployed_.get(), error_code::wrong_initialization);
    workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    deployed_ = true;
    wrapper_deployers_ = wrapper_deployers;
    keep_evers_ = keep_evers;
    // For clone deploy (old_token_version non empty), we need to check that sender is a correct WrappersConfig of a previous token version
    // For original deploy, we checking that sender is SuperRoot
    if (old_token_version) {
      require(!!evers_opt, error_code::missed_evers_cfg);
      auto evers = *evers_opt;
      auto hash = prepare<IWrappersConfig>(DWrappersConfig{.token_version_ = *old_token_version}, tvm_mycode()).second;
      auto old_wrappers_cfg = address::make_std(workchain_id_, hash);
      require(old_wrappers_cfg == int_sender(), error_code::not_correct_previous_clone_deploying_me);

      if (first_wic) {
        // WICs will be chain-cloned to the new version with new Wrappers deployment
        // Last WIC will call onWICsCloned callback
        // TODO: add expecting method gas usage calculation with gastogram
        require((evers.deploy + evers.setnext + evers.wic_keep) * wic_count + keep_evers < int_value().get(), error_code::not_enough_evers);
        IWICPtr(*first_wic)(_all_except(keep_evers)).cloneUpgrade(evers, {}, {}, {}, {}, token_version_, tvm_myaddr(), wrapper_deployers_);
        last_wic_ = last_wic; // We are waiting onWICsCloned from this old last_wic
      }
    } else {
      auto cfg = getConfig();
      require(int_sender() == cfg.super_root, error_code::only_super_root_may_deploy_me);
      // We are keeping first_wic_/last_wic_ empty here.
      return _all_except(keep_evers).with_void();
    }
  }

  void onWICsCloned(
    address_opt first_wic,
    address_opt last_wic,
    uint32      wic_count
  ) {
    require(last_wic_ && int_sender() == *last_wic_, error_code::unexpected_callback);
    first_wic_ = first_wic;
    last_wic_  = last_wic;
    wic_count_ = wic_count;

    tvm_rawreserve(keep_evers_.get(), rawreserve_flag::up_to);
    tvm_transfer(getConfig().super_root, 0, true, SEND_ALL_GAS | IGNORE_ACTION_ERRORS);
  }

  void addWrapperType(
    uint128 keep_evers,
    uint8   type,
    address wrapper_deployer
  ) {
    auto cfg = getConfig();
    require(int_sender() == cfg.super_root, error_code::message_sender_is_not_my_owner);
    require(type == wrapper_deployers_.size(), error_code::wrong_wrapper_type_num);
    wrapper_deployers_.push_back(wrapper_deployer);

    return _all_except(keep_evers).with_void();
  }

  void addWrapper(
    uint128       keep_evers,
    WICCloneEvers evers,
    string        symbol,
    uint8         type,
    cell          init_args
  ) {
    auto cfg = getConfig();
    require(int_sender() == cfg.super_root, error_code::message_sender_is_not_my_owner);
    require(type < wrapper_deployers_.size(), error_code::wrong_wrapper_type_num);

    auto deployer = wrapper_deployers_.get_at(type.get());
    auto [my_code_hash, my_code_depth] = my_code_hash_and_depth();
    cell salted_wic_code = tvm_add_code_salt<WICSalt>(
      {cfg.super_root, tvm_myaddr(), my_code_hash, my_code_depth}, cfg.wic_code);
    if (!last_wic_) {
      auto [init, hash] = prepare<IWIC>(DWIC{.symbol_=symbol}, salted_wic_code);
      IWICPtr wic(address::make_std(workchain_id_, hash));
      wic.deploy(init, Evers(evers.deploy.get())).onDeploy(evers.wic_keep, {}, {}, false, deployer, type, init_args);
      first_wic_ = wic.get();
      last_wic_ = wic.get();
      wic_count_ = 1;
    } else {
      auto [init, hash] = prepare<IWIC>(DWIC{.symbol_=symbol}, salted_wic_code);
      IWICPtr wic(address::make_std(workchain_id_, hash));
      wic.deploy(init, Evers(evers.deploy.get())).onDeploy(evers.wic_keep, {}, {}, false, deployer, type, init_args);
      IWICPtr(*last_wic_)(Evers(evers.setnext.get())).setNext({}, {}, wic.get());
      last_wic_ = wic.get();
      ++wic_count_;
    }

    return _all_except(keep_evers).with_void();
  }

  void unlistWrapper(address wic) {
    require(int_sender() == getConfig().super_root, error_code::message_sender_is_not_my_owner);
    IWICPtr(wic)(_remaining_ev()).unlist();
  }

  address cloneUpgrade(
    address_opt         answer_addr,
    uint128             keep_evers,
    uint128             clone_deploy_evers,
    WICCloneEvers       evers,
    uint32              new_token_version,
    dict_array<address> wrapper_deployers
  ) {
    require(int_sender() == getConfig().super_root, error_code::message_sender_is_not_my_owner);
    require(new_token_version > token_version_, error_code::wrong_token_version);
    require(wrapper_deployers.size() == wrapper_deployers_.size(), error_code::wrong_deployers_size);
    // TODO: add expecting method gas usage calculation with gastogram
    require((evers.deploy + evers.setnext + evers.wic_keep) * wic_count_ + keep_evers < clone_deploy_evers, error_code::not_enough_evers);
    require(clone_deploy_evers + keep_evers < tvm_balance(), error_code::not_enough_evers);

    auto [init, hash] = prepare<IWrappersConfig>(DWrappersConfig{.token_version_=new_token_version}, tvm_mycode());
    IWrappersConfigPtr clone(address::make_std(workchain_id_, hash));
    tvm_rawreserve(keep_evers.get(), rawreserve_flag::up_to);
    clone.deploy(init, Evers(clone_deploy_evers.get())).
      onDeploy(keep_evers, evers, token_version_, wrapper_deployers, first_wic_, last_wic_, wic_count_);

    // to send answer to the original caller (caller->SuperRoot->WrappersConfig->caller)
    if (answer_addr)
      set_int_sender(*answer_addr);
    set_int_return_flag(SEND_ALL_GAS);
    return clone.get();
  }

  WrappersConfigDetails getDetails() {
    auto [my_code_hash, my_code_depth] = my_code_hash_and_depth();
    return {
      token_version_, wrapper_deployers_, first_wic_, last_wic_, wic_count_,
      uint256(tvm_hash(
        tvm_add_code_salt<WICSalt>(
          {getConfig().super_root, tvm_myaddr(), my_code_hash, my_code_depth}, getConfig().wic_code)))
    };
  }

  WrappersConfigSalt getConfig() {
    return parse_chain_static<WrappersConfigSalt>(parser(tvm_code_salt()));
  }

  static std::pair<uint256, uint16> my_code_hash_and_depth() {
    cell cl = tvm_mycode();
    return { uint256(tvm_hash(cl)), uint16(cl.cdepth()) };
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  __always_inline static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    auto cfg = parse_chain_static<WrappersConfigSalt>(parser(tvm_code_salt()));
    auto [hdr, persist] = load_persistent_data<IWrappersConfig, void, DWrappersConfig>();
    if (tvm_balance() > static_cast<int>(persist.keep_evers_.get())) {
      tvm_rawreserve(persist.keep_evers_.get(), rawreserve_flag::up_to);
      tvm_transfer(cfg.super_root, 0, false, SEND_ALL_GAS | IGNORE_ACTION_ERRORS);
    }
    return 0;
  }
};

DEFINE_JSON_ABI(IWrappersConfig, DWrappersConfig, EWrappersConfig);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(WrappersConfig, IWrappersConfig, DWrappersConfig)
