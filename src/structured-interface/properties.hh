#pragma once

#include <cstdint>

#include <structured-interface/anchor.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/style.hh>
#include <structured-interface/vertices.hh>

#include <clean-core/span.hh>

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
/// NOTE: is usually an absolute coordinate
extern property_handle<tg::aabb2> aabb;

/// stores the text of controls like button, checkbox, or text itself
extern property_handle<cc::string_view> text;

/// stores an absolute position for this element
/// (coordinates are relative to parent but auto-layouting is disabled)
extern property_handle<tg::pos2> absolute_pos;

/// some elements support a fixed size (in pixels)
/// TODO: width / height separate?
extern property_handle<tg::size2> fixed_size;

/// if true, removes element and its children from input handling
extern property_handle<bool> no_input;

/// collapsed elements typically have no visible children
extern property_handle<bool> collapsed;

/// allows disabling input elements
extern property_handle<bool> enabled;

/// custom style class
extern property_handle<uint16_t> style_class;

/// if true, allows for editing its text
extern property_handle<bool> edit_text;

/// visibility state of the element (default is visible)
extern property_handle<si::style::visibility> visibility;

/// detached elements start their own logical UI trees even if they are children (e.g. tooltips, context menus, popovers)
extern property_handle<bool> detached;

/// a generic 8bit state property
extern property_handle<uint8_t> state_u8;

/// a generic 32bit float state property
extern property_handle<float> state_f32;

/// relative placement of an element (e.g. for tooltips)
extern property_handle<si::placement> placement;

/// triangle list for custom drawing
extern property_handle<cc::span<si::colored_vertex>> custom_triangles;

/// an entry of the style
extern property_handle<si::style::style_value> style_value;

namespace detail
{
/// sort index of a window (relative to its parent)
extern property_handle<int> window_idx;
}
}
}
