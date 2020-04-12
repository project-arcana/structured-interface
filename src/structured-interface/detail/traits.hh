#pragma once

#include <type_traits>

#include <clean-core/string_view.hh>

namespace si::detail
{
template <class T, class = void>
struct can_be_string_view_t : std::false_type
{
};
template <class T>
struct can_be_string_view_t<T, std::void_t<decltype(cc::string_view(std::declval<T>()))>> : std::true_type
{
};

template <class T>
static constexpr bool can_be_string_view = can_be_string_view_t<T>::value;
}
