#include "properties.hh"

#include <clean-core/map.hh>
#include <clean-core/string.hh>

#include <mutex>

// TODO: threadsafety!

namespace si::property
{
property_handle<cc::string_view> text;

}

void si::detail::init_default_properties()
{
    static std::once_flag once;
    std::call_once(once, [] {
        auto const add = [](auto& p, cc::string_view name) {
            p = std::decay_t<decltype(p)>::create("text");
            si::detail::register_typed_property(p.id(), p.type_id(), name);
        };

        add(si::property::text, "text");
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
