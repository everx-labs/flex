pragma ton-solidity >=0.61.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;

struct Tip3Config {
  string  name;         ///< Token name.
  string  symbol;       ///< Token short symbol.
  uint8   decimals;     ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint256 root_pubkey;  ///< Public key of RootTokenContract (or Wrapper) for the tip3 token.
  address root_address; ///< Address of RootTokenContract (or Wrapper) for the tip3 token.
}

contract NotifyStub {
    address m_superRoot;

    constructor (address superRoot) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
		tvm.accept();
        m_superRoot = superRoot;
    }

    function upgrade(TvmCell state) public virtual {
        require(msg.pubkey() == tvm.pubkey(), 101);
        TvmCell newcode = state.toSlice().loadRef();
        tvm.accept();
        tvm.commit();
        tvm.setcode(newcode);
        tvm.setCurrentCode(newcode);
        onCodeUpgrade();
    }

    function onCodeUpgrade() internal {
        tvm.resetStorage();
    }

    function checkAndTransfer() internal view inline  {
        if (address(this).balance > 101 ever)
            m_superRoot.transfer(100 ever);
    }

    function onXchgDealCompleted(
        bool       seller_is_taker, ///< Seller is a taker in deal
        address    pair,            ///< Address of XchgPair contract
        address    tip3root_major,  ///< Address of RootTokenContract for the major tip3 token
        address    tip3root_minor,  ///< Address of RootTokenContract for the minor tip3 token
        Tip3Config major_tip3cfg,   ///< Major tip3 configuration
        Tip3Config minor_tip3cfg,   ///< Minor tip3 configuration
        uint128    price_num,       ///< Token price numerator
        uint128    price_denum,     ///< Token price denominator
        uint128    amount           ///< Amount of major tip3 tokens in the deal
    ) public view functionID(10) {
        checkAndTransfer();
    }

    function onXchgOrderAdded(
      bool    sell,           ///< Is it a sell order
      address tip3root_major, ///< Address of RootTokenContract for the major tip3 token
      address tip3root_minor, ///< Address of RootTokenContract for the minor tip3 token
      uint128 price_num,      ///< Token price numerator
      uint128 price_denum,    ///< Token price denominator
      uint128 amount,         ///< Amount of major tip3 tokens added in the order
      uint128 sum_amount      ///< Summarized amount of major tokens rest in all orders for this price (sell or buy only)
    ) public view functionID(11) {
        checkAndTransfer();
    }

    function onXchgOrderCanceled(
      bool    sell,           ///< Is it a sell order
      address tip3root_major, ///< Address of RootTokenContract for the major tip3 token
      address tip3root_minor, ///< Address of RootTokenContract for the minor tip3 token
      uint128 price_num,      ///< Token price numerator
      uint128 price_denum,    ///< Token price denominator
      uint128 amount,         ///< Amount of major tip3 tokens canceled
      uint128 sum_amount      ///< Summarized amount of major tokens rest in all orders for this price (sell or buy only)
    ) public view functionID(12) {
        checkAndTransfer();
    }


}