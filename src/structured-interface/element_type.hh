#pragma once

#include <cstdint>

#include <clean-core/string_view.hh>

namespace si
{
enum class element_type : uint8_t
{
    // specials
    none = 0,

    // container
    window,
    tree_node,
    tabs,
    tab,
    grid,
    row,
    flow,
    container,
    canvas,

    // basics
    input,
    button,
    slider,
    checkbox,
    toggle,
    text,
    radio_button,
    dropdown,
    listbox,
    combobox,

    // custom are >= 128
    custom = 128
};

constexpr cc::string_view to_string(element_type t)
{
    if (t >= element_type::custom)
        return "custom"; // TODO: constexpr id

    switch (t)
    {
    case element_type::none:
        return "none";

    case element_type::window:
        return "window";
    case element_type::tree_node:
        return "tree_node";
    case element_type::tabs:
        return "tabs";
    case element_type::tab:
        return "tab";
    case element_type::grid:
        return "grid";
    case element_type::row:
        return "row";
    case element_type::flow:
        return "flow";
    case element_type::container:
        return "container";

    case element_type::input:
        return "input";
    case element_type::button:
        return "button";
    case element_type::slider:
        return "slider";
    case element_type::checkbox:
        return "checkbox";
    case element_type::toggle:
        return "toggle";
    case element_type::text:
        return "text";
    case element_type::radio_button:
        return "radio_button";
    case element_type::dropdown:
        return "dropdown";
    case element_type::listbox:
        return "listbox";
    case element_type::combobox:
        return "combobox";

    default:
        return "unknown";
    }
}
}
