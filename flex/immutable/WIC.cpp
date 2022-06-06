/** \file
 *  \brief Wrapper Index Contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "WIC.hpp"
#include "Wrapper.hpp"
#include "WrappersConfig.hpp"
#include "WrapperDeployer.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// Wrapper Index Contract implementation
class WIC final : public smart_interface<IWIC>, public DWIC {
  using data = DWIC;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IWIC, void)

  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner          = 100; ///< Authorization error
    static constexpr unsigned not_correct_previous_clone_deploying_me = 101; ///< Not a correct previous clone deploying me
    static constexpr unsigned not_correct_wrappers_cfg_deploying_me   = 102; ///< Not a correct WrappersConfig deploying me
    static constexpr unsigned incorrect_set_next_sender_clone         = 103; ///< Incorrect sender WIC clone in setNext call
    static constexpr unsigned empty_next_symbol                       = 104; ///< Empty `next` WIC symbol
    static constexpr unsigned wrong_wrapper_type                      = 105; ///< Wrapper type is out of wrapper deployers array
    static constexpr unsigned uninitialized                           = 106; ///< WIC uninitialized
    static constexpr unsigned wrong_prev_wic_in_chain                 = 107; ///< Wrong previous WIC in chain
    static constexpr unsigned wrong_cloned_wrappers_cfg               = 108; ///< Wrong cloned WrappersConfig
  };

  resumable<void> onDeploy(
    uint128     keep_evers,        ///< Keep evers in the contract
    address_opt old_wrappers_cfg,  ///< Old WrappersConfig address (empty if it is original listing deploy)
    address_opt old_wrapper,       ///< Old wrapper for this clone
    bool        keep_wrapper,      ///< If deployer is not changed (at cloning), keep_wrapper will be true.
    address     deployer,          ///< Wrapper deployer address
    uint8       type,              ///< Wrapper type
    cell        init_args          ///< Init args
  ) {
    auto cfg = getConfig();
    // For clone deploy (old_wrappers_cfg non empty), we need to check that sender is a correct Wrapper Index Contract.
    // For original listing deploy, we checking that sender is WrappersConfig
    if (old_wrappers_cfg) {
      auto expected_std_addr = prepare<IWIC>(DWIC{.symbol_ = symbol_}, resalt(cfg, *old_wrappers_cfg)).second;
      require(address::make_std(workchain_id_, expected_std_addr) == int_sender(), error_code::not_correct_previous_clone_deploying_me);
    } else {
      require(cfg.wrappers_cfg == int_sender(), error_code::not_correct_wrappers_cfg_deploying_me);
    }
    workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    deployer_  = deployer;
    type_      = type;
    init_args_ = init_args;
    if (old_wrapper && keep_wrapper) {
      wrapper_ = old_wrapper;

      // Transferring all except `keep_evers` to WrappersConfig
      tvm_rawreserve(keep_evers.get(), rawreserve_flag::up_to);
      tvm_transfer(cfg.wrappers_cfg, 0, false, SEND_ALL_GAS);
    } else {
      opt<addr_std_fixed> old = old_wrapper ? opt<addr_std_fixed>(*old_wrapper) : opt<addr_std_fixed>();
      cell mycode = tvm_mycode();
      slice trunc_code = __builtin_tvm_scutfirst(mycode.ctos(), mycode.ctos().sbits(), 2);
      cell unsalted_mycode = builder().stslice(trunc_code).endc();
      auto [wrapper, pubkey] =
        co_await IWrapperDeployerPtr(deployer)(_all_except(keep_evers)).deploy(init_args, unsalted_mycode);
      wrapper_ = wrapper;

      tvm_rawreserve(keep_evers.get(), rawreserve_flag::up_to);
      if (old) {
        IWrapperPtr(*old)(0_ev, SEND_ALL_GAS).
          setCloned(wrapper_, pubkey, cfg.wrappers_cfg, cfg.wrappers_cfg_code_hash, cfg.wrappers_cfg_code_depth);
      } else {
        // Transferring all except `keep_evers` to WrappersConfig
        tvm_transfer(cfg.wrappers_cfg, 0, false, SEND_ALL_GAS);
      }
    }
    co_return;
  }

  void setNext(
    address_opt old_wrappers_cfg, ///< Old WrappersConfig address in case of call from previous `next` WIC
    opt<string> next_symbol,      ///< Next WIC `symbol`. Must be set iff old_wrappers_cfg is set.
    address     next              ///< New `next` WIC in the linked list
  ) {
    auto cfg = getConfig();
    // setNext may be called by WrappersConfig, or by previous `next` WIC
    if (old_wrappers_cfg) {
      require(!!next_symbol, error_code::empty_next_symbol);
      auto expected_std_addr = prepare<IWIC>(DWIC{.symbol_ = *next_symbol}, resalt(cfg, *old_wrappers_cfg)).second;
      require(address::make_std(workchain_id_, expected_std_addr) == int_sender(), error_code::incorrect_set_next_sender_clone);
    } else {
      require(cfg.wrappers_cfg == int_sender(), error_code::message_sender_is_not_my_owner);
    }

    next_ = next;
  }

  void cloneUpgrade(
    WICCloneEvers       evers,
    opt<address>        first_clone,
    opt<address>        last_clone,
    opt<string>         prev_symbol,
    uint32              wic_count,
    uint32              token_version,
    address             new_wrappers_cfg,
    dict_array<address> wrapper_deployers
  ) {
    require(type_ < wrapper_deployers.size(), error_code::wrong_wrapper_type);
    require(deployer_ && type_ && init_args_, error_code::uninitialized);
    auto cfg = getConfig();
    if (prev_symbol) { // Chain call from previous WIC
      auto expected_std_addr = prepare<IWIC>(DWIC{.symbol_ = *prev_symbol}, tvm_mycode()).second;
      require(int_sender() == address::make_std(workchain_id_, expected_std_addr), error_code::wrong_prev_wic_in_chain);
    } else { // Starting call from WrappersConfig clone
      auto expected_std_addr = expected<IWrappersConfig>(
        DWrappersConfig{.token_version_=token_version}, cfg.wrappers_cfg_code_hash, cfg.wrappers_cfg_code_depth
      );
      auto addr = address::make_std(workchain_id_, expected_std_addr);
      require(int_sender() == addr && addr == new_wrappers_cfg, error_code::wrong_cloned_wrappers_cfg);
    }

    if (!unlisted_.get()) {
      cell newcode = resalt(cfg, new_wrappers_cfg);
      address deployer = wrapper_deployers.get_at(type_->get());
      auto old_wrappers_cfg = cfg.wrappers_cfg;
      auto [init, hash] = prepare<IWIC>(DWIC{.symbol_ = symbol_}, newcode);
      IWICPtr wic(address::make_std(workchain_id_, hash));
      // If deployer is not changed,
      bool keep_wrapper = (deployer == *deployer_);
      wic.deploy(init, Evers(evers.deploy.get())).
        onDeploy(evers.wic_keep, old_wrappers_cfg, wrapper_, keep_wrapper, deployer, *type_, *init_args_);
      if (last_clone)
        IWICPtr(*last_clone)(Evers(evers.setnext.get())).setNext(old_wrappers_cfg, symbol_, wic.get());
      if (!first_clone)
        first_clone = wic.get();
      last_clone = wic.get();
      ++wic_count;
    }

    tvm_rawreserve(evers.wic_keep.get(), rawreserve_flag::up_to);
    if (next_) {
      IWICPtr(*next_)(0_ev, SEND_ALL_GAS).
        cloneUpgrade(evers, first_clone, last_clone, symbol_, wic_count, token_version, new_wrappers_cfg, wrapper_deployers);
    } else {
      IWrappersConfigPtr(new_wrappers_cfg)(0_ev, SEND_ALL_GAS).onWICsCloned(first_clone, last_clone, wic_count);
    }
  }

  void unlist() {
    auto wrappers_cfg = getConfig().wrappers_cfg;
    require(wrappers_cfg == int_sender(), error_code::message_sender_is_not_my_owner);
    unlisted_ = true;
    tvm_transfer(wrappers_cfg, 0, false, SEND_REST_GAS_FROM_INCOMING);
  }

  /// Re-salt code with new configuration WICSalt
  cell resalt(WICSalt cfg, address wrappers_cfg) {
    auto code = tvm_mycode();
    auto code_sl = code.ctos();

    slice trunc_code = __builtin_tvm_scutfirst(code.ctos(), code_sl.sbits(), 2);
    auto salt = build_chain_static<WICSalt>({cfg.super_root, wrappers_cfg, cfg.wrappers_cfg_code_hash, cfg.wrappers_cfg_code_depth});
    return builder().stslice(trunc_code).stref(salt).endc();
  }

  WICDetails getDetails() {
    return { symbol_, workchain_id_, deployer_, wrapper_, type_, init_args_, next_, unlisted_.get() };
  }

  WICSalt getConfig() {
    return parse_chain_static<WICSalt>(parser(tvm_code_salt()));
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IWIC, DWIC, EWIC);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(WIC, IWIC, DWIC)
