#include "element_tree.hh"

#include <rich-log/log.hh>

#include <structured-interface/recorded_ui.hh>

static constexpr std::byte s_version_byte = std::byte(0x12);

bool si::element_tree::is_element(const si::element_tree::element& e) const
{
    if (_elements.empty())
        return false;
    return &_elements.front() <= &e && &e <= &_elements.back();
}

bool si::element_tree::has_property(const si::element_tree::element& e, si::untyped_property_handle prop) const
{
    CC_ASSERT(is_element(e) && "wrong tree? or accidental copy?");

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
    CC_ASSERT(is_element(e) && "wrong tree? or accidental copy?");

    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
            return {_packed_property_data.data() + p.value_start, size_t(p.value_size)};

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
    CC_ASSERT(is_element(e) && "wrong tree? or accidental copy?");

    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
            return {_packed_property_data.data() + p.value_start, size_t(p.value_size)};

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
    CC_ASSERT(is_element(e) && "wrong tree? or accidental copy?");

    for (auto const& p : packed_properties_of(e))
        if (p.id == prop)
        {
            CC_ASSERT(p.value_size == int(value.size()) && "property size mismatch");
            std::memcpy(_packed_property_data.data() + p.value_start, value.data(), value.size());
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
    auto raw_data = rui.raw_data();
    for (auto const& vp : v.properties)
    {
        auto& p = tree._packed_properties[vp.tree_prop_idx];
        p.id = vp.id;

        // this points into record buffer
        // and is immediately reassigned in the next loop
        // we only use this with reading access
        p.value_start = int(vp.value.data() - raw_data.data());
        p.value_size = int(vp.value.size());
    }

    // copy property data
    size_t idx = 0;
    for (auto& p : tree._packed_properties)
    {
        std::memcpy(tree._packed_property_data.data() + idx, raw_data.data() + p.value_start, p.value_size);
        p.value_start = idx;
        idx += p.value_size;
    }

    // create id lookup
    tree.create_element_map();

    return tree;
}

cc::vector<std::byte> si::element_tree::to_binary_data() const
{
    cc::vector<std::byte> data;
    data.push_back(s_version_byte);
    auto const add = [&](auto const& v) { data.push_back_range(cc::as_byte_span(v)); };
    auto const write_vec = [&](auto const& v) {
        add(v.size());
        add(v);
    };
    add(_root_count);
    write_vec(_elements);
    write_vec(_packed_properties);
    write_vec(_packed_property_data);
    write_vec(_dynamic_properties);
    return data;
}

si::element_tree si::element_tree::from_binary_data(cc::span<const std::byte> data)
{
    si::element_tree tree;

    if (data.size() >= 1 && data[0] == s_version_byte)
    {
        size_t idx = 1;
        auto read_size = [&] {
            CC_ASSERT(idx + sizeof(size_t) <= data.size());
            size_t v;
            std::memcpy(&v, data.data() + idx, sizeof(v));
            idx += sizeof(size_t);
            return v;
        };
        auto read_data = [&](auto& v) {
            v.resize(read_size());
            auto bs = v.size_bytes();
            CC_ASSERT(idx + bs <= data.size());
            std::memcpy(v.data(), data.data() + idx, bs);
            idx += bs;
        };

        tree._root_count = read_size();
        read_data(tree._elements);
        read_data(tree._packed_properties);
        read_data(tree._packed_property_data);
        read_data(tree._dynamic_properties);

        // create id lookup
        tree.create_element_map();
    }
    else
    {
        LOG_WARN("ui data has wrong version, ignoring deserialization (expected 0x{}, got 0x{})", s_version_byte, data.empty() ? std::byte(0) : data[0]);
    }

    return tree;
}

void si::element_tree::create_element_map()
{
    CC_ASSERT(_elements_by_id.empty() && "only works on clean tree");
    _elements_by_id.reserve(_elements.size());
    for (auto& e : _elements)
        _elements_by_id[e.id.id()] = &e;
}
