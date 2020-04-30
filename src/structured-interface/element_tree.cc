#include "element_tree.hh"

#include <structured-interface/recorded_ui.hh>

bool si::element_tree::has_property(const si::element_tree::element& e, si::untyped_property_handle prop) const
{
    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
            return true;

    auto idx = e.non_packed_properties_start;
    while (idx != -1)
    {
        auto const& p = reinterpret_cast<dynamic_property const&>(_dynamic_properties[idx]);
        if (p.id == prop)
            return true;

        idx = p.next_idx;
    }

    return false;
}

cc::span<std::byte> si::element_tree::get_property(const si::element_tree::element& e, si::untyped_property_handle prop)
{
    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
            return p.value;

    auto idx = e.non_packed_properties_start;
    while (idx != -1)
    {
        auto const& p = reinterpret_cast<dynamic_property const&>(_dynamic_properties[idx]);
        if (p.id == prop)
            return {_dynamic_properties.data() + idx + sizeof(dynamic_property), p.size};

        idx = p.next_idx;
    }

    CC_UNREACHABLE("property not found");
}

cc::span<const std::byte> si::element_tree::get_property(const si::element_tree::element& e, si::untyped_property_handle prop) const
{
    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
            return p.value;

    auto idx = e.non_packed_properties_start;
    while (idx != -1)
    {
        auto const& p = reinterpret_cast<dynamic_property const&>(_dynamic_properties[idx]);
        if (p.id == prop)
            return {_dynamic_properties.data() + idx + sizeof(dynamic_property), p.size};

        idx = p.next_idx;
    }

    CC_UNREACHABLE("property not found");
}

bool si::element_tree::set_property(si::element_tree::element& e, si::untyped_property_handle prop, cc::span<const std::byte> value)
{
    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
        {
            CC_ASSERT(p.value.size() == value.size() && "property size mismatch");
            std::memcpy(p.value.data(), value.data(), value.size());
            return false;
        }

    auto idx = e.non_packed_properties_start;
    while (idx != -1)
    {
        auto const& p = reinterpret_cast<dynamic_property const&>(_dynamic_properties[idx]);
        if (p.id == prop)
        {
            CC_ASSERT(p.size == value.size() && "property size mismatch");
            std::memcpy(_dynamic_properties.data() + idx + sizeof(dynamic_property), value.data(), value.size());
            return false;
        }

        idx = p.next_idx;
    }

    // allocate new property
    {
        auto new_idx = _dynamic_properties.size();
        _dynamic_properties.resize(_dynamic_properties.size() + sizeof(dynamic_property) + value.size());

        // init new prop
        auto& p = reinterpret_cast<dynamic_property&>(_dynamic_properties[new_idx]);
        p.id = prop;
        p.size = value.size();

        // add at front of non-packed props
        p.next_idx = e.non_packed_properties_start;
        e.non_packed_properties_start = new_idx;
        e.properties_count++;

        // copy value
        std::memcpy(_dynamic_properties.data() + new_idx + sizeof(dynamic_property), value.data(), value.size());

        return true;
    }
}

si::element_tree si::element_tree::from_record(const si::recorded_ui& rui)
{
    struct visitor
    {
        struct element_header
        {
            element_handle id;
            element_type type;
            int children = 0;
            int properties = 0;
            int parent_idx = -1;
            int child_idx = -1;

            int tree_idx = -1;
            int tree_child_start_idx = -1;
            int tree_property_start_idx = -1;
        };
        struct prop
        {
            untyped_property_handle id;
            int prop_idx = -1;
            cc::span<cc::byte const> value;

            int element_idx = -1;
            int tree_prop_idx = -1;
        };

        cc::vector<element_header> elements;
        cc::vector<size_t> elements_stack;
        cc::vector<prop> properties;
        size_t roots = 0;
        size_t property_data_size = 0;

        void start_element(size_t id, element_type type)
        {
            auto idx = elements.size();
            auto& e = elements.emplace_back();
            e.id = element_handle::from_id(id);
            e.type = type;

            if (!elements_stack.empty())
            {
                e.parent_idx = int(elements_stack.back());
                e.child_idx = elements[e.parent_idx].children;
                elements[e.parent_idx].children++;
            }
            else
            {
                roots++;
            }

            elements_stack.push_back(idx);
        }
        void property(size_t prop_id, cc::span<cc::byte const> value)
        {
            auto& p = properties.emplace_back();
            auto& e = elements[elements_stack.back()];

            p.id = untyped_property_handle::from_id(prop_id);
            p.value = value;
            p.element_idx = elements_stack.back();
            p.prop_idx = e.properties;

            e.properties++;

            property_data_size += value.size();
        }
        void end_element() { elements_stack.pop_back(); }

        void assign_indices()
        {
            auto root_idx = 0;
            auto prop_idx = 0;
            auto tree_idx = int(roots);
            CC_ASSERT(elements_stack.empty() && "should not be in stack phase");
            for (auto& e : elements)
            {
                // alloc children
                e.tree_child_start_idx = tree_idx;
                tree_idx += e.children;

                // alloc properties
                e.tree_property_start_idx = prop_idx;
                prop_idx += e.properties;

                if (e.parent_idx == -1) // root
                {
                    e.tree_idx = root_idx;
                    root_idx++;
                }
                else
                {
                    CC_ASSERT(e.child_idx >= 0);
                    auto const& p = elements[e.parent_idx];
                    CC_ASSERT(e.child_idx < p.children);

                    e.tree_idx = p.tree_child_start_idx + e.child_idx;
                }
            }
            for (auto& p : properties)
            {
                auto const& e = elements[p.element_idx];
                CC_ASSERT(0 <= p.prop_idx && p.prop_idx < e.properties);
                p.tree_prop_idx = e.tree_property_start_idx + p.prop_idx;
            }

            CC_ASSERT(prop_idx == int(properties.size()));
            CC_ASSERT(root_idx == int(roots));
            CC_ASSERT(tree_idx == int(elements.size()));
        }
    };

    // visit ui cmd buffer
    visitor v;
    rui.visit(v);

    // compute target indices
    v.assign_indices();

    // reserve tree
    element_tree tree;
    tree._elements.resize(v.elements.size());
    tree._packed_properties.resize(v.properties.size());
    tree._packed_property_data.resize(v.property_data_size);
    tree._root_count = v.roots;

    // copy elements
    for (auto const& ve : v.elements)
    {
        auto& e = tree._elements[ve.tree_idx];
        e.id = ve.id;
        e.type = ve.type;
        e.children_start = ve.tree_child_start_idx;
        e.children_count = ve.children;
        e.properties_start = ve.tree_property_start_idx;
        e.properties_count = ve.properties;
        e.packed_properties_count = ve.properties;
    }

    // copy properties
    for (auto const& vp : v.properties)
    {
        auto& p = tree._packed_properties[vp.tree_prop_idx];
        p.id = vp.id;

        // this points into record buffer
        // and is immediately reassigned in the next loop
        // we only use this with reading access
        p.value = {const_cast<std::byte*>(vp.value.data()), vp.value.size()};
    }

    // copy property data
    size_t idx = 0;
    for (auto& p : tree._packed_properties)
    {
        std::memcpy(tree._packed_property_data.data() + idx, p.value.data(), p.value.size());
        p.value = {tree._packed_property_data.data() + idx, p.value.size()};
        idx += p.value.size();
    }

    return tree;
}
