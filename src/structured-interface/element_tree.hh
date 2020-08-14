#pragma once

#include <clean-core/function_ref.hh>
#include <clean-core/map.hh>
#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <structured-interface/detail/record.hh>
#include <structured-interface/element_type.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/properties.hh>

namespace si
{
struct element_tree_element // outside so it can be forward declared
{
    element_handle id;
    element_type type;

    int parent_idx = -1;
    int children_start = 0;
    int children_count = 0;
    int properties_start = 0;
    int properties_count = 0;
    int packed_properties_count = 0;
    int non_packed_properties_start = -1; // byte offset into dynamic_properties
};
static_assert(sizeof(element_tree_element) == 8 * 5, "unexpected size");

// TODO: maybe preserve property references via chunk alloc
struct element_tree
{
    using element = element_tree_element;

    struct property
    {
        // TODO: maybe inline values below 128bit?
        untyped_property_handle id;
        int value_start = 0;
        int value_size = 0;
    };
    struct dynamic_property
    {
        untyped_property_handle id;
        int64_t next_idx; // byte offset into dynamic_properties
        size_t size;      // size in bytes, data starts after dynamic property
    };

    element_tree() = default;
    // move-only so property byte span remains valid
    element_tree(element_tree&&) = default;
    element_tree& operator=(element_tree&&) = default;
    element_tree(element_tree const&) = delete;
    element_tree& operator=(element_tree const&) = delete;

    // query API
    // TODO: measure if some of these should be inlined
public:
    /// returns true if element is part of this tree
    bool is_element(element const& e) const;

    element* parent_of(element const& e) { return e.parent_idx >= 0 ? &_elements[e.parent_idx] : nullptr; }
    element const* parent_of(element const& e) const { return e.parent_idx >= 0 ? &_elements[e.parent_idx] : nullptr; }

    // note: returns nullptr if not found
    element* get_element_by_id(element_handle id) { return _elements_by_id.get_or(id.id(), nullptr); }
    element const* get_element_by_id(element_handle id) const { return _elements_by_id.get_or(id.id(), nullptr); }

    cc::span<element> roots() { return {_elements.data(), _root_count}; }
    cc::span<element const> roots() const { return {_elements.data(), _root_count}; }

    cc::span<element> children_of(element const& e) { return {_elements.data() + e.children_start, size_t(e.children_count)}; }
    cc::span<element const> children_of(element const& e) const { return {_elements.data() + e.children_start, size_t(e.children_count)}; }

    cc::span<element> all_elements() { return _elements; }
    cc::span<element const> all_elements() const { return _elements; }

    cc::span<property> packed_properties_of(element& e)
    {
        return {_packed_properties.data() + e.properties_start, size_t(e.packed_properties_count)};
    }
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
    decltype(auto) get_property(element const& e, property_handle<T> prop)
    {
        return detail::property_read<T>(get_property(e, prop.untyped()));
    }
    /// queries the value of a property
    /// NOTE: asserts that the property exists
    /// CAUTION: reference can become invalid after set_property
    template <class T>
    decltype(auto) get_property(element const& e, property_handle<T> prop) const
    {
        return detail::property_read<T>(get_property(e, prop.untyped()));
    }
    /// queries the value of a property
    /// returns default_val if property not found
    /// CAUTION: reference can become invalid after set_property
    template <class T>
    decltype(auto) get_property_or(element const& e, property_handle<T> prop, tg::dont_deduce<T> const& default_val) const
    {
        // TODO: only look up once
        return has_property(e, prop) ? detail::property_read<T>(get_property(e, prop.untyped())) : default_val;
    }
    /// same as "element const&" version but also returns default_val if element is nullptr
    template <class T>
    decltype(auto) get_property_or(element const* e, property_handle<T> prop, tg::dont_deduce<T> const& default_val) const
    {
        return e ? this->get_property_or(*e, prop, default_val) : default_val;
    }
    /// queries the value of a property and writes it to "v"
    /// "v" is not touched if property is not found
    /// return true if property was found
    template <class T>
    bool get_property_to(element const& e, property_handle<T> prop, tg::dont_deduce<T>& v) const
    {
        // TODO: only look up once
        if (!has_property(e, prop))
            return false;

        v = detail::property_read<T>(get_property(e, prop.untyped()));
        return true;
    }
    /// same as "element const&" version but also returns false if element is nullptr
    template <class T>
    bool get_property_to(element const* e, property_handle<T> prop, tg::dont_deduce<T>& v) const
    {
        return e ? get_property_to(*e, prop, v) : false;
    }

    /// sets the value of a property (creating it in the dynamic area if it doesn't exist)
    /// returns true if property was newly created
    bool set_property(element& e, untyped_property_handle prop, cc::span<std::byte const> value);
    /// sets the value of a property (creating it in the dynamic area if it doesn't exist)
    /// returns true if property was newly created
    template <class T>
    bool set_property(element& e, property_handle<T> prop, tg::dont_deduce<T> const& value)
    {
        return set_property(e, prop.untyped(), cc::as_byte_span(value));
    }

    // creation
public:
    // TODO: reuse memory somehow
    static element_tree from_record(recorded_ui const& rui);

    // serialization
public:
    /// serializes this element_tree into byte data
    cc::vector<std::byte> to_binary_data() const;
    /// creates an element_tree from serialize data
    static element_tree from_binary_data(cc::span<std::byte const> data);

private:
    /// flattened list of hierarchical elements
    size_t _root_count = 0;
    cc::vector<element> _elements;
    cc::vector<property> _packed_properties;
    cc::vector<std::byte> _packed_property_data;
    cc::vector<std::byte> _dynamic_properties;
    cc::map<size_t, element*> _elements_by_id;

    // helper
private:
    void create_element_map();
};
}
