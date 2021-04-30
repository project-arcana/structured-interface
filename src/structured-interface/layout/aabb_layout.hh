#pragma once

#include <structured-interface/fwd.hh>

#include <typed-geometry/tg-lean.hh>

namespace si
{
struct aabb_layout_config
{
    float padding_left = 4.0f;
    float padding_right = 4.0f;
    float padding_top = 4.0f;
    float padding_bottom = 4.0f;
};

/// (re)computes si::property::aabb for all elements
void compute_aabb_layout(element_tree& elements, aabb_layout_config const& cfg = {});
}
