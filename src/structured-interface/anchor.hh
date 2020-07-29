#pragma once

#include <cstdint>

#include <typed-geometry/tg-lean.hh>

namespace si
{
enum class anchor : uint8_t
{
    none = 0,

    // horizontal
    left = 1,
    hcenter = 2,
    middle = hcenter,
    right = 3,
    mouse_x = 4,

    // vertical
    top = 1 << 4,
    vcenter = 2 << 4,
    bottom = 3 << 4,
    mouse_y = 4 << 4,

    // common combinations
    center = hcenter | vcenter,
    top_left = top | left,
    top_middle = top | middle,
    top_right = top | right,
    center_left = vcenter | left,
    center_right = vcenter | right,
    bottom_left = bottom | left,
    bottom_middle = bottom | middle,
    bottom_right = bottom | right,
    mouse = mouse_x | mouse_y,
};

/// evaluates the position specified by a certain anchor
inline tg::pos2 anchor_pos(anchor a, tg::aabb2 bb, tg::pos2 mouse_pos)
{
    auto ax = uint32_t(a) & 0x0F;
    auto ay = uint32_t(a) & 0xF0;

    auto x = bb.min.x;
    auto y = bb.min.y;

    // x
    if (ax == uint32_t(anchor::left))
        x = bb.min.x;
    else if (ax == uint32_t(anchor::right))
        x = bb.max.x;
    else if (ax == uint32_t(anchor::hcenter))
        x = (bb.min.x + bb.max.x) / 2;
    else if (ax == uint32_t(anchor::mouse_x))
        x = mouse_pos.x;

    // y
    if (ay == uint32_t(anchor::top))
        y = bb.min.y;
    else if (ay == uint32_t(anchor::bottom))
        y = bb.max.y;
    else if (ay == uint32_t(anchor::vcenter))
        y = (bb.min.y + bb.max.y) / 2;
    else if (ay == uint32_t(anchor::mouse_y))
        y = mouse_pos.y;

    return {x, y};
}

/// description for a relative placement of an object
/// TODO: specify overflow behavior
/// TODO: tooltip/popover configs that can be implicitly converted from placement
struct placement
{
    anchor ref_anchor;
    anchor this_anchor;

    tg::vec2 offset;

    /// computes the position described by this placement
    /// this_bb contains a position component that is used when part of the this_anchor is none
    tg::pos2 compute(tg::aabb2 ref_bb, tg::pos2 mouse_pos, tg::aabb2 this_bb) const;

    constexpr static placement tooltip_default(float dis = 8) { return {anchor::mouse, anchor::bottom_middle, {0, -dis}}; }
    constexpr static placement popover_default(float dis = 4) { return centered_right(dis); }
    constexpr static placement centered_above(float dis = 4) { return {anchor::top_middle, anchor::bottom_middle, {0, -dis}}; }
    constexpr static placement centered_below(float dis = 4) { return {anchor::bottom_middle, anchor::top_middle, {0, dis}}; }
    constexpr static placement centered_left(float dis = 4) { return {anchor::center_left, anchor::center_right, {-dis, 0}}; }
    constexpr static placement centered_right(float dis = 4) { return {anchor::center_right, anchor::center_left, {dis, 0}}; }
};
}
