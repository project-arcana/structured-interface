#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <clean-core/collection_traits.hh>
#include <clean-core/dont_deduce.hh>
#include <clean-core/is_contiguous_range.hh>
#include <clean-core/string_view.hh>

#include <structured-interface/detail/hash.hh>
#include <structured-interface/element_type.hh>
#include <structured-interface/handles.hh>

// NOTE: this header is included in si.hh
//       (it is "private" in the sense that users should not use it, but "public" in terms of includes)
namespace si::detail
{
enum class record_cmd : uint8_t
{
    start_element,
    end_element,
    property,
    external_property
};

inline std::byte*& get_record_buffer()
{
    thread_local std::byte* data = nullptr;
    return data;
}
inline std::byte*& get_record_buffer_end()
{
    thread_local std::byte* data = nullptr;
    return data;
}
inline element_handle& curr_element()
{
    thread_local element_handle element;
    return element;
}
void push_element(element_handle h);
void pop_element(element_handle h);

template <class T>
void record_write_raw(std::byte* d, T const& data)
{
    static_assert(std::is_trivially_copyable_v<T>);
    reinterpret_cast<T&>(*d) = data;
}

// does NOT include size parameter
template <class T>
size_t record_property_size_of(T const& data)
{
    if constexpr (cc::is_any_contiguous_range<T>)
        return data.size() * sizeof(data.data()[0]);
    else
        return sizeof(data);
}

// NOTE: record_write assumes enough space is available!
// TODO: extensible property serialization
template <class T>
void record_write_property(std::byte* d, T const& data)
{
    if constexpr (cc::is_any_contiguous_range<T>)
    {
        using element_t = std::decay_t<decltype(data.data()[0])>;
        static_assert(std::is_trivially_copyable_v<element_t>);
        std::memcpy(d, data.data(), data.size() * sizeof(element_t));
    }
    else
    {
        static_assert(std::is_trivially_copyable_v<T>);
        reinterpret_cast<T&>(*d) = data;
    }
}

template <class T>
auto property_read(cc::span<std::byte> data)
{
    if constexpr (std::is_same_v<T, cc::string_view>)
    {
        return cc::string_view(reinterpret_cast<char const*>(data.data()), data.size());
    }
    else if constexpr (cc::is_any_contiguous_range<T>)
    {
        using element_t = std::decay_t<decltype(std::declval<T>().data()[0])>;
        return cc::span<element_t>(reinterpret_cast<element_t*>(data.data()), data.size() / sizeof(element_t));
    }
    else
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return *reinterpret_cast<T*>(data.data());
    }
}
template <class T>
auto property_read(cc::span<std::byte const> data)
{
    if constexpr (std::is_same_v<T, cc::string_view>)
    {
        return cc::string_view(reinterpret_cast<char const*>(data.data()), data.size());
    }
    else if constexpr (cc::is_any_contiguous_range<T>)
    {
        using element_t = std::decay_t<decltype(std::declval<T>().data()[0])>;
        return cc::span<element_t const>(reinterpret_cast<element_t const*>(data.data()), data.size() / sizeof(element_t));
    }
    else
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return *reinterpret_cast<T const*>(data.data());
    }
}

inline std::byte* alloc_record_buffer_space(size_t s)
{
    auto& d = get_record_buffer();
    CC_ASSERT(d && "cannot build UI outside of recording sessions! (did you forget to call si::gui::record?)");

    if (d + s >= get_record_buffer_end())
    {
        CC_ASSERT(false && "implement me");
        // alloc new
    }

    auto const p = d;
    d += s;
    return p;
}

inline element_handle start_element(element_type type, element_handle id)
{
    si::detail::push_element(id);
    CC_ASSERT(id.is_valid());
    auto const d = alloc_record_buffer_space(1 + sizeof(id) + sizeof(type));
    record_write_raw(d + 0, record_cmd::start_element);
    record_write_raw(d + 1, type);
    record_write_raw(d + 2, id);
    return id;
}

template <class... Args>
element_handle start_element(element_type type, Args const&... id_args)
{
    return start_element(type, element_handle::create(type, id_args...));
}

inline void end_element(element_handle id)
{
    CC_ASSERT(id.is_valid());
    auto const d = alloc_record_buffer_space(1);
    record_write_raw(d, record_cmd::end_element);
    si::detail::pop_element(id);
}

template <class T>
void write_property(element_handle element, property_handle<T> prop, cc::dont_deduce<T const&> value)
{
    CC_ASSERT(prop.is_valid());
    CC_ASSERT(element.is_valid());
    CC_ASSERT(element == curr_element() && "TODO: implement external_property");

    // TODO: maybe 32bit is sufficient, 4GB+ data in an UI sounds weird
    auto const prop_size = detail::record_property_size_of(value);
    auto const d = alloc_record_buffer_space(1 + sizeof(prop.id()) + sizeof(prop_size) + prop_size);
    record_write_raw(d, record_cmd::property);
    record_write_raw(d + 1, prop.id());
    record_write_raw(d + 1 + sizeof(prop.id()), prop_size);
    detail::record_write_property(d + 1 + sizeof(prop.id()) + sizeof(prop_size), value);
}
}
