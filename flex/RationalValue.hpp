/** \file
 *  \brief Flex rational price (numerator / denominator)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Rational value - numerator/denominator
struct RationalValue {
  uint128 numerator() const { return num; }     ///< Numerator part of rational number
  uint128 denominator() const { return denum; } ///< Denominator part of rational number
  uint128 num;
  uint128 denum;
};
using price_t = RationalValue;

__always_inline
uint128 mul(uint128 amount, RationalValue price) {
  unsigned cost = __builtin_tvm_muldivr(amount.get(), price.numerator().get(), price.denominator().get());
  require((cost >> 128) == 0, error_code::integer_overflow);
  return uint128{cost};
}

__always_inline
uint128 operator*(uint128 l, RationalValue r) {
  return mul(l, r);
}

__always_inline
uint128 operator*(RationalValue l, uint128 r) {
  return mul(r, l);
}

} // namespace tvm

