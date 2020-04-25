#pragma once

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <structured-interface/element_type.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/properties.hh>

namespace si
{
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
    };
    struct property
    {
        // TODO: maybe inline values below 128bit?
        size_t id;
        cc::span<std::byte const> value;
    };

    // query API
public:
    cc::span<element const> roots() const { return {_elements.data(), _root_count}; }
    cc::span<element const> children_of(element const& e) const { return {_elements.data() + e.children_start, size_t(e.children_count)}; }
    cc::span<property const> properties_of(element const& e) const { return {_properties.data() + e.properties_start, size_t(e.properties_count)}; }

    // creation
public:
    // TODO: reuse memory somehow
    static element_tree from_record(recorded_ui const& rui);

private:
    /// flattened list of hierarchical elements
    size_t _root_count = 0;
    cc::vector<element> _elements;
    cc::vector<property> _properties;
    cc::vector<std::byte> _property_data;
};
}
