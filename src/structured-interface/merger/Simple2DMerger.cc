#include "Simple2DMerger.hh"

#include <typed-geometry/tg.hh>

#include <clean-core/strided_span.hh>

#include <structured-interface/element_tree.hh>
#include <structured-interface/input_state.hh>

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

void add_quad(si::Simple2DMerger::render_list& rl, tg::aabb2 bb, tg::color4 color, tg::aabb2 const& clip)
{
    // clipping
    auto cbb = intersection(bb, clip);
    if (!cbb.has_value())
        return;
    bb = cbb.value();

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
// TODO: clip
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

// returns nullptr if nothing found
si::element_tree_element* query_child_element_at(si::element_tree& tree, cc::span<si::element_tree_element> elements, tg::pos2 p)
{
    for (auto& c : cc::strided_span(elements).reversed())
    {
        auto bb = tree.get_property(c, si::property::aabb);

        if (contains(bb, p))
        {
            auto e = query_child_element_at(tree, tree.children_of(c), p);

            return e ? e : &c;
        }
    }

    return nullptr;
}
}

si::Simple2DMerger::Simple2DMerger()
{
    // TODO: configurable
    load_default_font();
}

si::element_tree si::Simple2DMerger::operator()(si::element_tree const&, si::element_tree&& ui, input_state& input)
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

    // step 2: input
    {
        // TODO: layers, events?

        // update hover
        if (is_lmb_down) // mouse down? copy from last
            input.hover_curr = input.hover_last;
        // otherwise search topmost
        else if (auto hc = query_child_element_at(ui, ui.roots(), mouse_pos))
            input.hover_curr = hc->id;

        // update pressed
        if (is_lmb_down)
            input.pressed_curr = input.hover_curr;
        else
            input.pressed_curr = {};

        input.mouse_pos = mouse_pos;
    }

    // step 3: render data
    {
        // TODO: reuse memory
        _render_data.lists.clear();
        _render_data.lists.emplace_back();

        for (auto& e : ui.roots())
            build_render_data(ui, e, input, viewport);
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

void si::Simple2DMerger::add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, tg::aabb2 const&)
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

tg::aabb2 si::Simple2DMerger::perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y)
{
    CC_ASSERT(e.type == element_type::checkbox);
    CC_ASSERT(e.children_count == 0 && "expected no children");

    float cx = x + padding_left;
    float cy = y + padding_top;
    auto txt = tree.get_property(e, si::property::text);

    auto bs = tg::round(font_size * 1.1f);

    auto tx = cx + bs + padding_left;
    tree.set_property(e, si::property::text_origin, tg::pos2(tx, cy));
    auto tbb = get_text_bounds(txt, tx, cy);

    auto bb = tg::aabb2({x, y}, tbb.max + tg::vec2(padding_right, padding_bottom));
    tree.set_property(e, si::property::aabb, bb);
    return bb;
}

void si::Simple2DMerger::render_checkbox(si::element_tree const& tree, si::element_tree_element const& e, si::input_state const& input, tg::aabb2 const& clip)
{
    CC_ASSERT(e.type == element_type::checkbox);

    auto bb = tree.get_property(e, si::property::aabb);
    auto bs = tg::round(font_size * 1.1f);
    auto is_hover = input.hover_curr == e.id;
    auto is_pressed = input.pressed_curr == e.id;
    auto is_checked = tree.get_property(e, si::property::state_u8) == uint8_t(true);

    // checkbox
    auto cbb = tg::aabb2(bb.min + tg::vec2(padding_left, padding_top), tg::size2(bs));
    add_quad(_render_data.lists.back(), cbb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    if (is_checked)
        add_quad(_render_data.lists.back(), shrink(cbb, tg::round(bs * 0.2f)), tg::color4(0, 0, 0, 0.6f), clip);

    // checkbox text
    render_text(tree, e, clip);
}

tg::aabb2 si::Simple2DMerger::perform_slider_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y)
{
    CC_ASSERT(e.type == element_type::slider);
    CC_ASSERT(e.children_count == 1 && "expected 1 child");
    auto& c = tree.children_of(e)[0];
    CC_ASSERT(c.type == element_type::slider_area);

    float cx = x + padding_left;
    float cy = y + padding_top;
    auto txt = tree.get_property(e, si::property::text);

    auto slider_width = 140.f;
    auto slider_height = font_size * 1.2f;
    tree.set_property(c, si::property::aabb, tg::aabb2({cx, cy}, tg::size2(slider_width, slider_height)));

    // value text
    {
        // TODO: clip
        auto txt = tree.get_property(c, si::property::text);
        auto tbb = get_text_bounds(txt, cx, cy);
        auto tx = cx + (slider_width - (tbb.max.x - tbb.min.x)) / 2;
        tree.set_property(c, si::property::text_origin, tg::pos2(tx, cy));
    }

    // slider text
    auto tx = cx + slider_width + padding_left;
    tree.set_property(e, si::property::text_origin, tg::pos2(tx, cy));
    auto tbb = get_text_bounds(txt, tx, cy);

    auto bb = tg::aabb2({x, y}, tbb.max + tg::vec2(padding_right, padding_bottom));
    tree.set_property(e, si::property::aabb, bb);
    return bb;
}

void si::Simple2DMerger::render_slider(si::element_tree const& tree, si::element_tree_element const& e, si::input_state const& input, tg::aabb2 const& clip)
{
    CC_ASSERT(e.type == element_type::slider);
    auto& c = tree.children_of(e)[0];

    auto is_hover = input.hover_curr == c.id;
    auto is_pressed = input.pressed_curr == c.id;

    // slider box
    auto sbb = tree.get_property(c, si::property::aabb);
    add_quad(_render_data.lists.back(), sbb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);

    // slider knob
    {
        auto khw = 7;
        auto t = tree.get_property(c, si::property::state_f32);
        auto x = tg::mix(sbb.min.x + khw + 1, sbb.max.x - khw - 1, t);
        add_quad(_render_data.lists.back(), tg::aabb2({x - khw, sbb.min.y + 1}, {x + khw, sbb.max.y - 1}), tg::color4(1, 1, 1, 0.4f), clip);
    }

    // value text
    render_text(tree, c, clip);

    // slider text
    render_text(tree, e, clip);
}

tg::aabb2 si::Simple2DMerger::perform_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y)
{
    // TODO: early out is possible

    // special types
    switch (e.type)
    {
    case element_type::checkbox:
        return perform_checkbox_layout(tree, e, x, y);

    case element_type::slider:
        return perform_slider_layout(tree, e, x, y);

    default:
        break; // generic handling for all other types
    }


    float cx = x + padding_left;
    float cy = y + padding_top;
    float w = 0;

    if (e.type == element_type::checkbox)
    {
        cx += font_size * 1.5f;
    }

    if (tree.has_property(e, si::property::text))
    {
        auto txt = tree.get_property(e, si::property::text);
        tree.set_property(e, si::property::text_origin, tg::pos2(cx, cy));
        auto bb = get_text_bounds(txt, cx, cy);
        auto [tw, th] = tg::size_of(bb);
        w = tg::max(w, tw);
        cy += th;
    }

    for (auto& c : tree.children_of(e))
    {
        auto bb = perform_layout(tree, c, cx, cy);
        auto s = tg::size_of(bb);
        cy += s.height;
        cy += padding_child;
        w = tg::max(w, s.width);
    }

    // DEBUG!
    w = tg::max(w, 16);

    auto start = tg::pos2(x, y);
    auto end = tg::pos2(cx + w + padding_right, cy + padding_bottom);
    auto bb = tg::aabb2(start, end);
    tree.set_property(e, si::property::aabb, bb);
    return bb;
}

void si::Simple2DMerger::render_text(si::element_tree const& tree, si::element_tree_element const& e, tg::aabb2 const& clip)
{
    auto txt = tree.get_property(e, si::property::text);
    auto tp = tree.get_property(e, si::property::text_origin);

    add_text_render_data(_render_data.lists.back(), txt, tp.x, tp.y, clip);
}

void si::Simple2DMerger::build_render_data(si::element_tree const& tree, si::element_tree_element const& e, input_state const& input, tg::aabb2 const& clip)
{
    auto bb = tree.get_property(e, si::property::aabb);
    auto bb_clip = intersection(bb, clip);
    if (!bb_clip.has_value())
        return; // early out: out of clip

    // special handling
    switch (e.type)
    {
    case element_type::checkbox:
        return render_checkbox(tree, e, input, clip);
    case element_type::slider:
        return render_slider(tree, e, input, clip);
    default:
        break; // generic handling
    }

    auto has_bg = false;
    if (e.type == element_type::button)
        has_bg = true;
    // TODO: more generic?

    if (has_bg)
    {
        auto is_hover = input.hover_curr == e.id;
        auto is_pressed = input.pressed_curr == e.id;
        add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    }

    if (tree.has_property(e, si::property::text))
        render_text(tree, e, clip);

    for (auto& c : tree.children_of(e))
        build_render_data(tree, c, input, bb_clip.value());
}