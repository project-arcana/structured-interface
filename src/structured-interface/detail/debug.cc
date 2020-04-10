#include "debug.hh"

#include <iomanip>
#include <iostream>

#include <clean-core/string.hh>

#include <structured-interface/properties.hh>
#include <structured-interface/recorded_ui.hh>

void si::debug::print(recorded_ui const& r)
{
    struct printer
    {
        int indent = 0;
        cc::vector<size_t> id_stack;
        std::string indent_str() const { return std::string(indent * 2, ' '); }
        void start_element(size_t id, element_type type)
        {
            std::cout << indent_str() << "[" << cc::string(to_string(type)).c_str() << "] id: " << id << std::endl;
            ++indent;
            id_stack.push_back(id);
        }
        void property(size_t prop_id, cc::span<cc::byte const> value)
        {
            auto name = si::detail::get_property_name_from_id(prop_id);
            auto type = si::detail::get_property_type_from_id(prop_id);
            std::cout << indent_str() << "- " << std::string_view(name.data(), name.size()) << ": ";
            if (type == cc::type_id<cc::string_view>())
            {
                std::cout << "\"";
                std::cout << std::string_view(reinterpret_cast<char const*>(value.data()), value.size());
                std::cout << "\"";
            }
            else
            {
                std::cout << std::hex;
                for (auto v : value)
                    std::cout << " " << std::setw(2) << std::setfill('0') << unsigned(v);
                std::cout << std::dec;
            }
            std::cout << std::endl;
        }
        void end_element()
        {
            --indent;
            id_stack.pop_back();
        }
    };
    r.visit(printer{});
}
