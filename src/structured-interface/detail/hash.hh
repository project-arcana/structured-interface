#pragma once

#include <clean-core/xxHash.hh>

#include <structured-interface/detail/traits.hh>

namespace si::detail
{
template <class T>
void combine_hash(uint64_t& hash, T const& v)
{
    if constexpr (can_be_string_view<T const>)
    {
        auto sv = cc::string_view(v);
        hash = cc::hash_xxh3(cc::span(sv).as_bytes(), hash);
    }
    else
    {
        static_assert(std::is_trivially_copyable_v<T>, "only string-like and trivially copyable types can be hashed for now");
        hash = cc::hash_xxh3(cc::span<T const>(v).as_bytes(), hash);
    }
}

template <class... Args>
uint64_t make_hash(uint64_t seed, Args const&... args)
{
    static_assert(sizeof...(Args) > 0, "must provide at least one argument");
    auto h = seed;
    (combine_hash(h, args), ...);
    return h;
}
}
