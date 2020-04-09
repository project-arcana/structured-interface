#pragma once

#include <cstddef>

#include <clean-core/fwd.hh>
#include <clean-core/string_view.hh>
#include <clean-core/xxHash.hh>

namespace si
{
struct element_handle
{
    element_handle() = default;

    bool is_valid() const { return _id > 0; }
    size_t id() const { return _id; }

    bool operator==(element_handle h) const { return _id == h._id; }
    bool operator!=(element_handle h) const { return _id != h._id; }

    static element_handle create(cc::string_view s, cc::hash_t seed)
    {
        return element_handle(cc::hash_xxh3({(std::byte const*)s.data(), s.size()}, seed));
    }
    static element_handle create(char const* s, cc::hash_t seed) { return create(cc::string_view(s), seed); }
    // static element_handle create(char const* s, cc::hash_t seed) { return create(cc::string_view(s), seed); }

private:
    explicit element_handle(size_t id) : _id(id) {}

    size_t _id = 0;
};

struct property_handle
{
    property_handle() = default;

    bool is_valid() const { return _id > 0; }
    size_t id() const { return _id; }

    bool operator==(property_handle h) const { return _id == h._id; }
    bool operator!=(property_handle h) const { return _id != h._id; }

private:
    size_t _id = 0;
};
}

template <>
struct cc::hash<si::element_handle>
{
    [[nodiscard]] hash_t operator()(si::element_handle const& value) const noexcept { return value.id(); }
};
