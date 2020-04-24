#pragma once

#include <cstddef>

#include <clean-core/fwd.hh>
#include <clean-core/type_id.hh>

#include <structured-interface/detail/hash.hh>

namespace si
{
namespace detail
{
inline cc::hash_t& id_seed()
{
    thread_local cc::hash_t seed = 0x51;
    return seed;
}
}

struct element_handle
{
    element_handle() = default;

    bool is_valid() const { return _id > 0; }
    size_t id() const { return _id; }

    bool operator==(element_handle h) const { return _id == h._id; }
    bool operator!=(element_handle h) const { return _id != h._id; }

    template <class... Args>
    static element_handle create(Args const&... args)
    {
        return element_handle(si::detail::make_hash(detail::id_seed(), args...));
    }
    static element_handle from_id(size_t id) { return element_handle(id); }

private:
    explicit element_handle(size_t id) : _id(id) {}

    size_t _id = 0;
};

template <class T>
struct property_handle
{
    property_handle() = default;

    bool is_valid() const { return _id > 0; }
    size_t id() const { return _id; }
    cc::type_id_t type_id() const { return cc::type_id<T>(); }

    bool operator==(property_handle h) const { return _id == h._id; }
    bool operator!=(property_handle h) const { return _id != h._id; }

    static property_handle create(cc::string_view name) { return property_handle(cc::hash_xxh3(cc::span(name).as_bytes(), 0x46464646)); }
    static property_handle from_id(size_t id) { return property_handle(id); }

private:
    explicit property_handle(size_t id) : _id(id) {}

    size_t _id = 0;
};
}

template <>
struct cc::hash<si::element_handle>
{
    [[nodiscard]] hash_t operator()(si::element_handle const& value) const noexcept { return value.id(); }
};
template <class T>
struct cc::hash<si::property_handle<T>>
{
    [[nodiscard]] hash_t operator()(si::property_handle<T> const& value) const noexcept { return value.id(); }
};
