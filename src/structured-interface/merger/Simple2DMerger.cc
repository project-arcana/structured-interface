#include "Simple2DMerger.hh"

#include <typed-geometry/tg.hh>

#include <structured-interface/element_tree.hh>

namespace
{
uint32_t to_rgba8(tg::color4 const& c)
{
    auto r = uint32_t(tg::clamp(int(256 * c.r), 0, 255));
    auto g = uint32_t(tg::clamp(int(256 * c.g), 0, 255));
    auto b = uint32_t(tg::clamp(int(256 * c.b), 0, 255));
    auto a = uint32_t(tg::clamp(int(256 * c.a), 0, 255));
    return (a << 24) | (b << 16) | (g << 8) | r;
}

// TODO: clip
void add_quad(si::Simple2DMerger::render_list& rl, tg::aabb2 const& bb, tg::color4 color)
{
    if (rl.cmds.empty())
    {
        CC_ASSERT(rl.indices.empty());
        rl.cmds.emplace_back();
    }

    auto& cmd = rl.cmds.back();
    CC_ASSERT(cmd.indices_start + cmd.indices_count == rl.indices.size());

    auto add_vertex = [&](tg::pos2 p) -> int {
        auto idx = int(rl.vertices.size());
        auto& v = rl.vertices.emplace_back();
        v.pos = p;
        v.uv = {0, 0}; // DEBUG
        v.color = to_rgba8(color);
        return idx;
    };

    auto v00 = add_vertex({bb.min.x, bb.min.y});
    auto v10 = add_vertex({bb.max.x, bb.min.y});
    auto v01 = add_vertex({bb.min.x, bb.max.y});
    auto v11 = add_vertex({bb.max.x, bb.max.y});

    rl.indices.push_back(v00);
    rl.indices.push_back(v10);
    rl.indices.push_back(v11);

    rl.indices.push_back(v00);
    rl.indices.push_back(v11);
    rl.indices.push_back(v01);

    cmd.indices_count += 6;
}
void add_quad_uv(si::Simple2DMerger::render_list& rl, tg::aabb2 const& bb, tg::aabb2 const& uv, tg::color4 color)
{
    if (rl.cmds.empty())
    {
        CC_ASSERT(rl.indices.empty());
        rl.cmds.emplace_back();
    }

    auto& cmd = rl.cmds.back();
    CC_ASSERT(cmd.indices_start + cmd.indices_count == rl.indices.size());

    auto add_vertex = [&](tg::pos2 p, tg::pos2 uv) -> int {
        auto idx = int(rl.vertices.size());
        auto& v = rl.vertices.emplace_back();
        v.pos = p;
        v.uv = uv;
        v.color = to_rgba8(color);
        return idx;
    };

    auto v00 = add_vertex({bb.min.x, bb.min.y}, {uv.min.x, uv.min.y});
    auto v10 = add_vertex({bb.max.x, bb.min.y}, {uv.max.x, uv.min.y});
    auto v01 = add_vertex({bb.min.x, bb.max.y}, {uv.min.x, uv.max.y});
    auto v11 = add_vertex({bb.max.x, bb.max.y}, {uv.max.x, uv.max.y});

    rl.indices.push_back(v00);
    rl.indices.push_back(v10);
    rl.indices.push_back(v11);

    rl.indices.push_back(v00);
    rl.indices.push_back(v11);
    rl.indices.push_back(v01);

    cmd.indices_count += 6;
}
}

si::Simple2DMerger::Simple2DMerger()
{
    // TODO: configurable
    load_default_font();
}

si::element_tree si::Simple2DMerger::operator()(si::element_tree const&, si::element_tree&& ui)
{
    // step 1: layout
    {
        float x = padding_left;
        float y = padding_top;

        for (auto& e : ui.roots())
        {
            auto bb = perform_layout(ui, e, x, y);
            y += tg::size_of(bb).height;
            y += padding_child;
        }
    }

    // step 2: render data
    {
        // TODO: reuse memory
        _render_data.lists.clear();
        _render_data.lists.emplace_back();

        for (auto& e : ui.roots())
            build_render_data(ui, e, viewport);
    }

    return cc::move(ui);
}

tg::aabb2 si::Simple2DMerger::get_text_bounds(cc::string_view txt, float x, float y)
{
    auto s = font_size / _font.ref_size;
    auto ox = x;

    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        x += _font.glyphs[c].advance * s;
    }

    return {{ox, y}, {x, y + _font.baseline_height * s}};
}

void si::Simple2DMerger::render_text(render_list& rl, cc::string_view txt, float x, float y, tg::aabb2 const&)
{
    auto s = font_size / _font.ref_size;

    // baseline
    auto bx = x;
    auto by = y + _font.ascender * s;

    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        auto const& gi = _font.glyphs[c];

        if (gi.height > 0) // ignore invisible chars
        {
            auto pmin = tg::pos2(bx + gi.bearingX * s, by - gi.bearingY * s);
            auto pmax = pmin + tg::vec2(gi.width * s, gi.height * s);
            add_quad_uv(rl, {pmin, pmax}, gi.uv, font_color);
        }

        bx += gi.advance * s;
    }
}

tg::aabb2 si::Simple2DMerger::perform_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y)
{
    // TODO: early out is possible

    float cx = x + padding_left;
    float cy = y + padding_top;
    float w = 0;
    float h = 0;

    if (tree.has_property(e, si::property::text))
    {
        auto txt = tree.get_property(e, si::property::text);
        auto bb = get_text_bounds(txt, cx, cy);
        auto [tw, th] = tg::size_of(bb);
        w = tg::max(w, tw);
        h += th;
        cy += th;
    }

    for (auto& c : tree.children_of(e))
    {
        auto bb = perform_layout(tree, c, cx, cy);
        auto s = tg::size_of(bb);
        cy += s.height;
        cy += padding_child;
        w = tg::max(w, s.width);
        h += s.height;
        h += padding_child;
    }

    // DEBUG!
    h = tg::max(h, 10);
    w = tg::max(w, 100);

    auto start = tg::pos2(x, y);
    auto end = tg::pos2(cx + w + padding_right, cy + padding_bottom);
    auto bb = tg::aabb2(start, end);
    tree.set_property(e, si::property::aabb, bb);
    return bb;
}

void si::Simple2DMerger::build_render_data(si::element_tree const& tree, si::element_tree_element const& e, tg::aabb2 const& clip)
{
    auto bb = tree.get_property(e, si::property::aabb);
    auto child_clip = intersection(bb, clip);
    if (!child_clip.has_value())
        return; // early out: out of clip

    add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 1, 0.2f));

    if (tree.has_property(e, si::property::text))
    {
        auto txt = tree.get_property(e, si::property::text);
        auto x = bb.min.x + padding_left;
        auto y = bb.min.y + padding_top;
        render_text(_render_data.lists.back(), txt, x, y, clip);
    }

    for (auto& c : tree.children_of(e))
        build_render_data(tree, c, child_clip.value());
}
