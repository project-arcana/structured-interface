#pragma once

#include <clean-core/flags.hh>

namespace si
{
enum class window_options
{
    start_collapsed,
};

CC_FLAGS_ENUM(window_options);

enum class collapsible_group_options
{
    start_collapsed,
};

CC_FLAGS_ENUM(collapsible_group_options);

constexpr struct
{
    constexpr operator cc::flags<collapsible_group_options>() const { return collapsible_group_options::start_collapsed; }
    constexpr operator cc::flags<window_options>() const { return window_options::start_collapsed; }
} start_collapsed;
}
