/*
 * Minimal in-repo replacement for the external irmv_core package.
 *
 * bind_this: bind a member function pointer to `this`, leaving every
 * argument position as a std::placeholders placeholder so the resulting
 * std::function can be called with arbitrary arguments.
 *
 * Implemented with std::index_sequence and the standard placeholders
 * (_1.._N) instead of the original custom placeholder machinery.
 */

#ifndef URDFAPPROXGEOM_IRMV_BIND_THIS_H
#define URDFAPPROXGEOM_IRMV_BIND_THIS_H

#include <functional>
#include <type_traits>
#include <utility>

namespace irmv_core {
namespace bot_common {
namespace detail {

template <class R, class U, class... Args, std::size_t... Is>
auto bind_this_impl(R (U::*p)(Args...), U* pp, std::index_sequence<Is...>)
    -> decltype(std::bind(p, pp, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4,
                          std::placeholders::_5, std::placeholders::_6,
                          std::placeholders::_7, std::placeholders::_8,
                          std::placeholders::_9, std::placeholders::_10,
                          std::placeholders::_11, std::placeholders::_12,
                          std::placeholders::_13, std::placeholders::_14,
                          std::placeholders::_15, std::placeholders::_16)) {
    return std::bind(p, pp, std::placeholders::_1, std::placeholders::_2,
                     std::placeholders::_3, std::placeholders::_4,
                     std::placeholders::_5, std::placeholders::_6,
                     std::placeholders::_7, std::placeholders::_8,
                     std::placeholders::_9, std::placeholders::_10,
                     std::placeholders::_11, std::placeholders::_12,
                     std::placeholders::_13, std::placeholders::_14,
                     std::placeholders::_15, std::placeholders::_16);
}

template <class R, class U, class... Args, std::size_t... Is>
auto bind_this_impl(R (U::*p)(Args...) const, U* pp, std::index_sequence<Is...>)
    -> decltype(std::bind(p, pp, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4,
                          std::placeholders::_5, std::placeholders::_6,
                          std::placeholders::_7, std::placeholders::_8,
                          std::placeholders::_9, std::placeholders::_10,
                          std::placeholders::_11, std::placeholders::_12,
                          std::placeholders::_13, std::placeholders::_14,
                          std::placeholders::_15, std::placeholders::_16)) {
    return std::bind(p, pp, std::placeholders::_1, std::placeholders::_2,
                     std::placeholders::_3, std::placeholders::_4,
                     std::placeholders::_5, std::placeholders::_6,
                     std::placeholders::_7, std::placeholders::_8,
                     std::placeholders::_9, std::placeholders::_10,
                     std::placeholders::_11, std::placeholders::_12,
                     std::placeholders::_13, std::placeholders::_14,
                     std::placeholders::_15, std::placeholders::_16);
}

} // namespace detail

// Bind member function to this; all argument positions remain placeholders.
template <class R, class U, class... Args>
auto bind_this(R (U::*p)(Args...), U* pp)
    -> decltype(detail::bind_this_impl(p, pp,
                                       std::make_index_sequence<sizeof...(Args)>{})) {
    return detail::bind_this_impl(p, pp, std::make_index_sequence<sizeof...(Args)>{});
}

template <class R, class U, class... Args>
auto bind_this(R (U::*p)(Args...) const, U* pp)
    -> decltype(detail::bind_this_impl(p, pp,
                                       std::make_index_sequence<sizeof...(Args)>{})) {
    return detail::bind_this_impl(p, pp, std::make_index_sequence<sizeof...(Args)>{});
}

} // namespace bot_common
} // namespace irmv_core

#endif // URDFAPPROXGEOM_IRMV_BIND_THIS_H
