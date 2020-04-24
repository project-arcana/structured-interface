#include "element_tree.hh"

#include <structured-interface/recorded_ui.hh>

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
            size_t id;
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

            p.id = prop_id;
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
    tree._properties.resize(v.properties.size());
    tree._property_data.resize(v.property_data_size);
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
    }

    // copy properties
    for (auto const& vp : v.properties)
    {
        auto& p = tree._properties[vp.tree_prop_idx];
        p.id = vp.id;
        p.value = vp.value; // points into record buffer
    }

    // copy property data
    size_t idx = 0;
    for (auto& p : tree._properties)
    {
        std::memcpy(tree._property_data.data() + idx, p.value.data(), p.value.size());
        p.value = {tree._property_data.data() + idx, p.value.size()};
        idx += p.value.size();
    }

    return tree;
}
