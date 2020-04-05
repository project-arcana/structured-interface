#include "debug.hh"

#include <iomanip>
#include <iostream>

#include <structured-interface/recorded_ui.hh>

void si::debug::print(recorded_ui const& r)
{
    struct printer
    {
        int indent = 0;
        cc::vector<size_t> id_stack;
        std::string indent_str() const { return std::string(indent * 2, ' '); }
        void start_element(size_t id)
        {
            std::cout << indent_str() << "[element] id: " << id << std::endl;
            ++indent;
            id_stack.push_back(id);
        }
        void property(size_t prop_id, cc::span<cc::byte const> value)
        {
            std::cout << indent_str() << "property: " << prop_id << " =";
            std::cout << std::hex;
            for (auto v : value)
                std::cout << " " << std::setw(2) << std::setfill('0') << unsigned(v);
            std::cout << std::dec;
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
