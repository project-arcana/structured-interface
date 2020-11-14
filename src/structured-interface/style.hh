#pragma once

#include <cstdint>

#include <typed-geometry/tg-lean.hh>

#include <clean-core/assert.hh>

namespace si::style
{
enum class visibility : uint8_t
{
    visible, ///< visible and takes up space
    hidden,  ///< invisible but takes up space
    none,    ///< invisible and does not take up space
};

enum class positioning : uint8_t
{
    normal,   ///< top-down or left-right depending on layout
    absolute, ///< uses absolute coordinates (relative to parent though)
};

enum class layout : uint8_t
{
    top_down,
    left_right,
    // TODO: more stuff like flow
};

enum class box_type : uint8_t
{
    content_box, ///< width/height are WITHOUT padding and border
    padding_box, ///< width/height are WITH padding but WITHOUT border
    border_box,  ///< width/height are WITH padding and border
};

enum class value_type : uint8_t
{
    auto_,     ///< inferred from layouting or otherwise
    explicit_, ///< combination of percentage and absolute
    // TODO: inherit?
};

enum class overflow : uint8_t
{
    visible, ///< nothing is clipped
    hidden,  ///< clipped
    // TODO: scrollbars
};

enum class font_align : uint8_t
{
    left,
    center,
    right,
};

struct value
{
    value_type type = value_type::auto_;
    float absolute = 0;
    float relative = 0; // 1.0 means 100% of parent

    bool is_auto() const { return type == value_type::auto_; }
    bool is_explicit() const { return type == value_type::explicit_; }
    bool is_resolved() const { return type == value_type::explicit_ && relative == 0; }
    bool has_percentage() const { return type == value_type::explicit_ && relative != 0; }

    float resolve(float auto_val, float perc_ref_val)
    {
        absolute = eval(auto_val, perc_ref_val);
        type = value_type::explicit_;
        relative = 0;
        return absolute;
    }

    float get() const
    {
        CC_ASSERT(type == value_type::explicit_ && relative == 0 && "not resolved?");
        return absolute;
    }

    value() = default;
    value(float v)
    {
        type = value_type::explicit_;
        absolute = v;
    }

    void set_relative(float factor)
    {
        type = value_type::explicit_;
        absolute = 0;
        relative = factor;
    }

    [[nodiscard]] float eval(float auto_val, float perc_ref_val) const
    {
        switch (type)
        {
        case value_type::auto_:
            return auto_val;
        case value_type::explicit_:
            return absolute + relative * perc_ref_val;
        default:
            CC_UNREACHABLE("unknown value type");
        }
    }

    bool operator==(float val) const { return type == value_type::explicit_ && absolute == val && relative == 0; }
    bool operator!=(float val) const { return !operator==(val); }
};

struct margin
{
    value top;
    value right;
    value bottom;
    value left;

    margin() = default;
    margin(float thickness) : top(thickness), right(thickness), bottom(thickness), left(thickness) {}
    margin(float vertical, float horizontal) : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    margin(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}
};

struct padding
{
    value top;
    value right;
    value bottom;
    value left;

    padding() = default;
    padding(float thickness) : top(thickness), right(thickness), bottom(thickness), left(thickness) {}
    padding(float vertical, float horizontal) : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    padding(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}
};

struct border
{
    value top;
    value right;
    value bottom;
    value left;
    tg::color4 color = tg::color4::black;
    value radius; // per side?

    border() = default;
    border(float thickness, tg::color4 color = tg::color4::black, float r = 0)
      : top(thickness), right(thickness), bottom(thickness), left(thickness), color(color), radius(r)
    {
    }
    border(float t, float r, float b, float l, tg::color4 color = tg::color4::black, float rad = 0)
      : top(t), right(r), bottom(b), left(l), color(color), radius(rad)
    {
    }
};

struct bounds
{
    value left;
    value right;
    value top;
    value bottom;
    value width;
    value min_width;
    value max_width;
    value height;
    value min_height;
    value max_height;
    bool fill_width = false;
    bool fill_height = false;
};

struct font
{
    font_align align = font_align::left;
    value size = 20.f;
    tg::color4 color = tg::color4::black;
    // TODO: font family, modifiers
};

// TODO: images, gradients, etc.
// TODO: blurred?
struct background
{
    tg::color4 color = tg::color4::transparent;

    background() = default;
    background(tg::color4 c) : color(c) {}
    background(tg::color3 c) : color(c, 1.f) {}
};

enum class style_entry : uint32_t
{
    invalid,

    left_abs,
    left_rel,

    top_abs,
    top_rel,

    width_abs,
    width_rel,

    height_abs,
    height_rel,

    // TODO: more
};

struct style_value
{
    style_entry entry = style_entry::invalid;
    float value = 0;
};
}
