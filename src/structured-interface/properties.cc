#include "properties.hh"

#include <clean-core/map.hh>
#include <clean-core/string.hh>

#include <mutex>

// TODO: threadsafety!

namespace si::property
{
property_handle<tg::aabb2> aabb;
property_handle<cc::string_view> text;
property_handle<tg::pos2> absolute_pos;
property_handle<bool> collapsed;
property_handle<bool> detached;
property_handle<bool> enabled;
property_handle<bool> no_input;
property_handle<bool> edit_text;
property_handle<uint16_t> style_class;
property_handle<cc::uint8> state_u8;
property_handle<float> state_f32;
property_handle<si::placement> placement;
property_handle<si::style::visibility> visibility;
property_handle<tg::size2> fixed_size;
property_handle<cc::span<si::colored_vertex>> custom_triangles;
namespace detail
{
property_handle<int> window_idx;
}
}

void si::detail::init_default_properties()
{
    static std::once_flag once;
    std::call_once(once, [] {
        auto const add = [](auto& p, cc::string_view name) {
            p = std::decay_t<decltype(p)>::create(name);
            si::detail::register_typed_property(p.id(), p.type_id(), name);
        };

        add(si::property::aabb, "aabb");
        add(si::property::text, "text");
        add(si::property::collapsed, "collapsed");
        add(si::property::absolute_pos, "absolute_pos");
        add(si::property::detached, "detached");
        add(si::property::enabled, "enabled");
        add(si::property::no_input, "no_input");
        add(si::property::style_class, "style_class");
        add(si::property::edit_text, "edit_text");
        add(si::property::state_u8, "state_u8");
        add(si::property::state_f32, "state_f32");
        add(si::property::placement, "placement");
        add(si::property::visibility, "visibility");
        add(si::property::fixed_size, "fixed_size");
        add(si::property::custom_triangles, "custom_triangles");

        add(si::property::detail::window_idx, "window_idx");
    });
}

namespace
{
struct prop_info
{
    cc::string name;
    cc::type_id_t type;
};

cc::map<size_t, prop_info> s_prop_by_id;
}

void si::detail::register_typed_property(size_t id, cc::type_id_t type, cc::string_view name)
{
    CC_ASSERT(id != 0 && "invalid id");
    CC_ASSERT(!s_prop_by_id.contains_key(id) && "already registered");

    auto& pi = s_prop_by_id[id];
    pi.name = name;
    pi.type = type;
}

cc::string_view si::detail::get_property_name_from_id(size_t id)
{
    CC_ASSERT(s_prop_by_id.contains_key(id) && "unknown property");
    return s_prop_by_id.get(id).name;
}

cc::type_id_t si::detail::get_property_type_from_id(size_t id)
{
    CC_ASSERT(s_prop_by_id.contains_key(id) && "unknown property");
    return s_prop_by_id.get(id).type;
}
