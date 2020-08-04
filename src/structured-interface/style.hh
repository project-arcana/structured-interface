#pragma once

#include <cstdint>

#include <typed-geometry/tg-lean.hh>

namespace si::style
{
enum class visibility : uint8_t
{
    visible, ///< visible and takes up space
    hidden,  ///< invisible but takes up space
    none     ///< invisible and does not take up space
};

enum class layout : uint8_t
{
    top_down,
    left_right,
    // TODO: more stuff like flow
};

// TODO: % value?
struct margin
{
    float top = 0;
    float right = 0;
    float bottom = 0;
    float left = 0;

    margin() = default;
    margin(float thickness) : top(thickness), right(thickness), bottom(thickness), left(thickness) {}
    margin(float vertical, float horizontal) : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    margin(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}
    void set_vertical(float t)
    {
        top = t;
        bottom = t;
    }
    void set_horizontal(float t)
    {
        left = t;
        right = t;
    }
};

struct padding
{
    float top = 0;
    float right = 0;
    float bottom = 0;
    float left = 0;

    padding() = default;
    padding(float thickness) : top(thickness), right(thickness), bottom(thickness), left(thickness) {}
    padding(float vertical, float horizontal) : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    padding(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}
    void set_vertical(float t)
    {
        top = t;
        bottom = t;
    }
    void set_horizontal(float t)
    {
        left = t;
        right = t;
    }
};

struct border
{
    float top = 0;
    float right = 0;
    float bottom = 0;
    float left = 0;
    tg::color4 color = tg::color4::black;
    float radius = 0; // per side?

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

struct font
{
    float size = 20.f;
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
}
