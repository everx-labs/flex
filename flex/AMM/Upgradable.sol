pragma ton-solidity >=0.35.0;

abstract contract Upgradable {
    /*
     * Set code
     */

    /// @notice Allows to upgrade contract code and data.
    /// @param state Root cell with StateInit structure of the new contract.
    /// Remark: only code is used from this structure.
    function upgrade(TvmCell state) public virtual {
        require(msg.pubkey() == tvm.pubkey(), 100);
        TvmCell newcode = state.toSlice().loadRef();
        tvm.accept();
        tvm.commit();
        tvm.setcode(newcode);
        tvm.setCurrentCode(newcode);
        onCodeUpgrade();
    }

    function onCodeUpgrade() internal virtual;
}