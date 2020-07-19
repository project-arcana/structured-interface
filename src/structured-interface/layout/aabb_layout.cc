#include "aabb_layout.hh"

#include <typed-geometry/tg.hh>

#include <structured-interface/element_tree.hh>

namespace
{
tg::aabb2 impl_aabb_layout(si::element_tree& tree, si::element_tree::element& e, float x, float y, const si::aabb_layout_config& cfg)
{
    float cx = x + cfg.padding_left;
    float cy = y + cfg.padding_top;
    float w = 0;
    float h = 0;

    for (auto& c : tree.children_of(e))
    {
        auto bb = impl_aabb_layout(tree, c, cx, cy, cfg);
        auto s = tg::size_of(bb);
        cy += s.height;
        w = tg::max(w, s.width);
        h += s.height;
    }

    // DEBUG!
    h = tg::max(h, 10);
    w = tg::max(w, 100);

    auto start = tg::pos2(x, y);
    auto end = tg::pos2(cx + w + cfg.padding_right, cy + cfg.padding_bottom);
    auto bb = tg::aabb2(start, end);
    tree.set_property(e, si::property::aabb, bb);
    return bb;
}
}

void si::compute_aabb_layout(si::element_tree& elements, const si::aabb_layout_config& cfg)
{
    float x = cfg.padding_left;
    float y = cfg.padding_top;

    for (auto& e : elements.roots())
    {
        auto bb = impl_aabb_layout(elements, e, x, y, cfg);
        y += tg::size_of(bb).height;
    }
}
