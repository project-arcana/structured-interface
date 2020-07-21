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
/// stores a 2D AABB that is the result of layouting
extern property_handle<tg::aabb2> aabb;

/// stores the text of controls like button, checkbox, or text itself
extern property_handle<cc::string_view> text;

/// stores the (x,y) origin of layouted text
/// NOTE: this is the typographical origin and does not correlate directly to text aabb origin
extern property_handle<tg::pos2> text_origin;

/// if true, removes element and its children from input handling
extern property_handle<bool> no_input;

/// a generic 8bit state property
extern property_handle<cc::uint8> state_u8;
}
}
