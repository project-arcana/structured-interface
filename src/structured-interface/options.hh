#pragma once

#include <clean-core/flags.hh>

namespace si
{
enum class window_option
{
    start_collapsed,
};

CC_FLAGS_ENUM(window_option);

enum class collapsible_group_option
{
    start_collapsed,
};

CC_FLAGS_ENUM(collapsible_group_option);

enum class checkbox_option
{
    enabled,
    disabled,
};

CC_FLAGS_ENUM(checkbox_option);

enum class toggle_option
{
    enabled,
    disabled,
};

CC_FLAGS_ENUM(toggle_option);

enum class button_option
{
    enabled,
    disabled,
};

CC_FLAGS_ENUM(button_option);

enum class slider_option
{
    enabled,
    disabled,
};

CC_FLAGS_ENUM(slider_option);

enum class radio_button_option
{
    enabled,
    disabled,
};

CC_FLAGS_ENUM(radio_button_option);

static constexpr struct
{
    constexpr operator cc::flags<collapsible_group_option>() const { return collapsible_group_option::start_collapsed; }
    constexpr operator cc::flags<window_option>() const { return window_option::start_collapsed; }
} start_collapsed;

static constexpr struct enabled_t
{
    constexpr operator cc::flags<checkbox_option>() const { return _value ? checkbox_option::enabled : checkbox_option::disabled; }
    constexpr operator cc::flags<toggle_option>() const { return _value ? toggle_option::enabled : toggle_option::disabled; }
    constexpr operator cc::flags<button_option>() const { return _value ? button_option::enabled : button_option::disabled; }
    constexpr operator cc::flags<slider_option>() const { return _value ? slider_option::enabled : slider_option::disabled; }
    constexpr operator cc::flags<radio_button_option>() const { return _value ? radio_button_option::enabled : radio_button_option::disabled; }

    constexpr enabled_t operator()(bool value) const
    {
        enabled_t v;
        v._value = value;
        return v;
    }

private:
    bool _value = true;
} enabled;

static constexpr struct disabled_t
{
    constexpr operator cc::flags<checkbox_option>() const { return !_value ? checkbox_option::enabled : checkbox_option::disabled; }
    constexpr operator cc::flags<toggle_option>() const { return !_value ? toggle_option::enabled : toggle_option::disabled; }
    constexpr operator cc::flags<button_option>() const { return !_value ? button_option::enabled : button_option::disabled; }
    constexpr operator cc::flags<radio_button_option>() const { return !_value ? radio_button_option::enabled : radio_button_option::disabled; }
    constexpr operator cc::flags<slider_option>() const { return !_value ? slider_option::enabled : slider_option::disabled; }

    constexpr disabled_t operator()(bool value) const
    {
        disabled_t v;
        v._value = value;
        return v;
    }

private:
    bool _value = true;
} disabled;
}
