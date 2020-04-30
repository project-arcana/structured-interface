#pragma once

#include <structured-interface/handles.hh>

#include <typed-geometry/tg-lean.hh>

namespace si
{
namespace detail
{
void init_default_properties(); ///< called in recorder.cc (start_recording)
void register_typed_property(size_t id, cc::type_id_t type, cc::string_view name);
cc::string_view get_property_name_from_id(size_t id);
cc::type_id_t get_property_type_from_id(size_t id);
}

template <class T>
property_handle<T> register_property(cc::string_view name)
{
    CC_ASSERT(!name.empty() && "property name must be non-empty");
    auto h = property_handle<T>::create(name);
    si::detail::register_typed_property(h.id(), h.type_id(), name);
    return h;
}

template <class T>
cc::string_view query_property_name(property_handle<T> prop)
{
    return si::detail::get_property_name_from_id(prop.id());
}

// NOTE: these are just default properties and in no way special
// NOTE: all properties added here must be initialized in detail::init_default_properties
namespace property
{
extern property_handle<cc::string_view> text;
extern property_handle<tg::aabb2> aabb;
}
}
