#pragma once

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <structured-interface/element_type.hh>
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

    // query API
public:
    cc::span<element const> children_of(element const& e) const { return {_elements.data() + e.children_start, size_t(e.children_count)}; }

private:
    /// flattened list of hierachical elements
    cc::vector<element> _elements;
    cc::vector<std::byte> _property_data;
};
}
