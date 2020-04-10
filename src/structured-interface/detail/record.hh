#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

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

// NOTE: record_write assumes enough space is available!
template <class T>
void record_write(std::byte* d, T const& data)
{
    static_assert(std::is_trivially_copyable_v<T>);
    reinterpret_cast<T&>(*d) = data;
}
inline void record_write(std::byte* d, cc::string_view data)
{
    reinterpret_cast<size_t&>(*d) = data.size();
    std::memcpy(d + sizeof(size_t), data.data(), data.size());
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

template <class... Args>
element_handle start_element(element_type type, Args const&... id_args)
{
    auto id = element_handle::create(type, id_args...);
    CC_ASSERT(id.is_valid());
    auto const d = alloc_record_buffer_space(1 + sizeof(id) + sizeof(type));
    record_write(d + 0, record_cmd::start_element);
    record_write(d + 1, type);
    record_write(d + 2, id);
    return id;
}

inline void end_element(element_handle id)
{
    auto const d = alloc_record_buffer_space(1);
    record_write(d, record_cmd::end_element);
}

inline void write_property(element_handle element, property_handle<cc::string_view> prop, cc::string_view value)
{
    CC_ASSERT(prop.is_valid());
    // TODO: check if element current and write externally
    (void)element;

    auto const d = alloc_record_buffer_space(1 + sizeof(prop.id()) + sizeof(value.size()) + value.size());
    record_write(d, record_cmd::property);
    record_write(d + 1, prop.id());
    // NOTE: size is written in record_write with string_view
    record_write(d + 1 + sizeof(prop.id()), value);
}

template <class T>
void write_property(element_handle element, property_handle<T> prop, T const& value)
{
    CC_ASSERT(prop.is_valid());
    // TODO: check if element current and write externally
    (void)element;

    // TODO: extensible property serialization
    static_assert(std::is_trivially_copyable_v<T>);

    auto const d = alloc_record_buffer_space(1 + sizeof(prop.id()) + sizeof(sizeof(value)) + sizeof(value));
    record_write(d, record_cmd::property);
    record_write(d + 1, prop.id());
    record_write(d + 1 + sizeof(prop.id()), sizeof(value));
    record_write(d + 1 + sizeof(prop.id()) + sizeof(sizeof(value)), value);
}

bool was_element_pressed();
}
