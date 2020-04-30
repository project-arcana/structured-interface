#pragma once

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <structured-interface/element_type.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/properties.hh>

namespace si
{
// TODO: maybe preserve property references via chunk alloc
struct element_tree
{
    struct element
    {
        element_handle id;
        element_type type;

        int children_start = 0;
        int children_count = 0;
        int properties_start = 0;
        int properties_count = 0;
        int packed_properties_count = 0;
        int non_packed_properties_start = -1; // byte offset into dynamic_properties
    };
    struct property
    {
        // TODO: maybe inline values below 128bit?
        untyped_property_handle id;
        cc::span<std::byte> value;
    };
    struct dynamic_property
    {
        untyped_property_handle id;
        int64_t next_idx; // byte offset into dynamic_properties
        size_t size;      // size in bytes, data starts after dynamic property
    };

    // query API
    // TODO: measure if some of these should be inlined
public:
    cc::span<element> roots() { return {_elements.data(), _root_count}; }
    cc::span<element const> roots() const { return {_elements.data(), _root_count}; }

    cc::span<element> children_of(element const& e) { return {_elements.data() + e.children_start, size_t(e.children_count)}; }
    cc::span<element const> children_of(element const& e) const { return {_elements.data() + e.children_start, size_t(e.children_count)}; }

    cc::span<property const> packed_properties_of(element const& e) const
    {
        return {_packed_properties.data() + e.properties_start, size_t(e.packed_properties_count)};
    }

    /// returns true if the element has the given property (either in the packed or dynamic area)
    bool has_property(element const& e, untyped_property_handle prop) const;

    /// queries the value of a property
    /// NOTE: asserts that the property exists
    cc::span<std::byte> get_property(element const& e, untyped_property_handle prop);
    /// queries the value of a property
    /// NOTE: asserts that the property exists
    cc::span<std::byte const> get_property(element const& e, untyped_property_handle prop) const;

    /// queries the value of a property
    /// NOTE: asserts that the property exists
    /// CAUTION: reference can become invalid after set_property
    template <class T>
    T& get_property(element const& e, property_handle<T> prop)
    {
        return *static_cast<T*>(get_property(e, prop.untyped()));
    }
    /// queries the value of a property
    /// NOTE: asserts that the property exists
    /// CAUTION: reference can become invalid after set_property
    template <class T>
    T const& get_property(element const& e, property_handle<T> prop) const
    {
        return *static_cast<T const*>(get_property(e, prop.untyped()));
    }

    /// sets the value of a property (creating it in the dynamic area if it doesn't exist)
    /// returns true if property was newly created
    bool set_property(element& e, untyped_property_handle prop, cc::span<std::byte const> value);
    /// sets the value of a property (creating it in the dynamic area if it doesn't exist)
    /// returns true if property was newly created
    template <class T>
    bool set_property(element& e, property_handle<T> prop, T const& value)
    {
        return set_property(e, prop.untyped(), cc::span<T const>(value).as_bytes());
    }

    // creation
public:
    // TODO: reuse memory somehow
    static element_tree from_record(recorded_ui const& rui);

private:
    /// flattened list of hierarchical elements
    size_t _root_count = 0;
    cc::vector<element> _elements;
    cc::vector<property> _packed_properties;
    cc::vector<std::byte> _packed_property_data;
    cc::vector<std::byte> _dynamic_properties;
};
}
