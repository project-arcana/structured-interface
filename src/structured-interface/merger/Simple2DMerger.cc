#include "Simple2DMerger.hh"

#include <rich-log/log.hh> // DEBUG

#include <algorithm> // sort

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
}

si::element_tree_element* si::Simple2DMerger::query_input_element_at(tg::pos2 p) const
{
    for (auto i = int(_layout_roots.size()) - 1; i >= 0; --i)
        if (auto e = query_input_child_element_at(_layout_roots[i], p))
            return e;
    return nullptr;
}

si::element_tree_element* si::Simple2DMerger::query_input_child_element_at(int layout_idx, tg::pos2 p) const
{
    auto const& le = _layout_tree[layout_idx];

    if (le.no_input || !le.is_visible)
        return nullptr; // does not receive input

    if (!contains(le.bounds, p))
        return nullptr; // TODO: proper handling of extended children (like in context menus)

    auto cs = le.child_start;
    auto ce = le.child_start + le.child_count;
    for (auto i = ce - 1; i >= cs; --i)
        if (auto e = query_input_child_element_at(i, p))
            return e;

    return le.element;
}

si::Simple2DMerger::Simple2DMerger()
{
    // TODO: configurable
    load_default_font();
}

si::element_tree si::Simple2DMerger::operator()(si::element_tree const& prev_ui, si::element_tree&& ui, input_state& input)
{
    // step 0: prepare data
    _prev_ui = &prev_ui;
    _layout_tree.clear();
    _layout_roots.clear();
    _layout_detached_roots.clear();
    _layout_original_roots = 0;
    _deferred_placements.clear();

    // step 1: layout
    {
        _layout_tree.reserve(ui.all_elements().size()); // might be reshuffled but is max size
        _layout_tree.resize(ui.roots().size());         // pre-alloc roots as there is not perform_layout for them

        // actual layouting
        perform_child_layout_default(ui, -1, ui.roots(), padding_left, padding_top);
        CC_ASSERT(_layout_original_roots <= int(ui.roots().size()));

        // finalize roots
        // NOTE: must be here so detached > windows
        for (auto i : _layout_detached_roots)
            _layout_roots.push_back(i);

        // resolve constraints
        resolve_deferred_placements(ui);
    }

    // step 2: input
    {
        // TODO: layers, events?

        // mouse pos
        input.mouse_pos = mouse_pos;
        input.mouse_delta = mouse_pos - prev_mouse_pos;
        prev_mouse_pos = mouse_pos;

        // update hover
        if (is_lmb_down) // mouse down? copy from last
            input.hover_curr = input.hover_last;
        // otherwise search topmost
        else if (auto hc = query_input_element_at(mouse_pos))
            input.hover_curr = hc->id;

        // update pressed
        if (is_lmb_down)
        {
            input.pressed_curr = input.hover_curr;
            drag_distance += length(input.mouse_delta);
            input.is_drag = drag_distance > 10; // configurable?
        }
        else
        {
            input.pressed_curr = {};
            drag_distance = 0;

            // NOTE: is_drag must persists one frame!
            if (!input.pressed_last.is_valid())
                input.is_drag = false;
        }
    }

    // step 3: render data
    {
        // TODO: reuse memory
        _render_data.lists.clear();
        _render_data.lists.emplace_back();

        // contains normal ui elements and detached ones
        for (auto i : _layout_roots)
            build_render_data(ui, _layout_tree[i], input, viewport);
    }

    // step 4: some cleanup
    _prev_ui = nullptr;

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

tg::aabb2 si::Simple2DMerger::perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y)
{
    CC_ASSERT(e.type == element_type::checkbox);

    float cx = x + padding_left;
    float cy = y + padding_top;
    auto txt = tree.get_property(e, si::property::text);

    auto bs = tg::round(font_size * 1.1f);

    auto tx = cx + bs + padding_left;
    _layout_tree[layout_idx].text_origin = {tx, cy};
    auto tbb = get_text_bounds(txt, tx, cy);

    // children (e.g. tooltips)
    perform_child_layout_default(tree, layout_idx, tree.children_of(e), cx, cy);

    return tg::aabb2({x, y}, tbb.max + tg::vec2(padding_right, padding_bottom));
}

void si::Simple2DMerger::render_checkbox(si::element_tree const& tree, layouted_element const& le, si::input_state const& input, tg::aabb2 const& clip)
{
    CC_ASSERT(le.element->type == element_type::checkbox);

    auto eid = le.element->id;
    auto bb = le.bounds;
    auto bs = tg::round(font_size * 1.1f);
    auto is_hover = input.hover_curr == eid;
    auto is_pressed = input.pressed_curr == eid;
    auto is_checked = tree.get_property(*le.element, si::property::state_u8) == uint8_t(true);

    // checkbox
    auto cbb = tg::aabb2(bb.min + tg::vec2(padding_left, padding_top), tg::size2(bs));
    add_quad(_render_data.lists.back(), cbb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    if (is_checked)
        add_quad(_render_data.lists.back(), shrink(cbb, tg::round(bs * 0.2f)), tg::color4(0, 0, 0, 0.6f), clip);

    // checkbox text
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start, le.child_start + le.child_count, input, clip);
}

tg::aabb2 si::Simple2DMerger::perform_slider_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y)
{
    CC_ASSERT(e.type == element_type::slider);
    CC_ASSERT(e.children_count >= 1 && "expected at least one child");
    auto& c = tree.children_of(e)[0];
    CC_ASSERT(c.type == element_type::slider_area);

    auto& cle = _layout_tree[add_child_layout_element(layout_idx)];
    cle.element = &c;

    float cx = x + padding_left;
    float cy = y + padding_top;
    auto txt = tree.get_property(e, si::property::text);

    auto slider_width = 140.f;
    auto slider_height = font_size * 1.2f;
    cle.bounds = tg::aabb2({cx, cy}, tg::size2(slider_width, slider_height));
    tree.set_property(c, si::property::aabb, cle.bounds);

    // value text
    {
        // TODO: clip
        auto txt = tree.get_property(c, si::property::text);
        auto tbb = get_text_bounds(txt, cx, cy);
        auto tx = cx + (slider_width - (tbb.max.x - tbb.min.x)) / 2;
        cle.text_origin = {tx, cy};
    }

    // slider text
    auto tx = cx + slider_width + padding_left;
    _layout_tree[layout_idx].text_origin = {tx, cy};
    auto tbb = get_text_bounds(txt, tx, cy);

    // children (e.g. tooltips)
    perform_child_layout_default(tree, layout_idx, tree.children_of(e).subspan(1), cx, cy);

    return tg::aabb2({x, y}, tbb.max + tg::vec2(padding_right, padding_bottom));
}

void si::Simple2DMerger::render_slider(si::element_tree const& tree, layouted_element const& le, si::input_state const& input, tg::aabb2 const& clip)
{
    CC_ASSERT(le.element->type == element_type::slider);
    auto& c = tree.children_of(*le.element)[0];

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
    render_text(tree, _layout_tree[le.child_start], clip);

    // slider text
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start + 1, le.child_start + le.child_count, input, clip);
}

tg::aabb2 si::Simple2DMerger::perform_window_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y)
{
    CC_ASSERT(e.type == element_type::window);
    CC_ASSERT(e.children_count >= 1 && "expected at least one child");
    auto& c = tree.children_of(e)[0];
    CC_ASSERT(c.type == element_type::clickable_area);

    auto& cle = _layout_tree[add_child_layout_element(layout_idx)];
    cle.element = &c;

    float cx = x + padding_left;
    float cy = y + padding_top;

    // title
    auto txt = tree.get_property(e, si::property::text);
    _layout_tree[layout_idx].text_origin = {cx, cy};
    auto tbb = get_text_bounds(txt, cx, cy);

    // content
    cy = tbb.max.y + padding_bottom + padding_top;
    auto cbb = perform_child_layout_default(tree, layout_idx, tree.children_of(e).subspan(1), cx, cy);

    // bb
    auto maxx = tg::max(tbb.max.x, cbb.max.x);
    auto maxy = cbb.max.y;
    auto bb = tg::aabb2({x, y}, {maxx + padding_right, maxy + padding_bottom});

    // clickable area
    cle.bounds = {{x, y}, {bb.max.x, tbb.max.y + padding_bottom}};
    tree.set_property(c, si::property::aabb, cle.bounds);

    return bb;
}

void si::Simple2DMerger::render_window(si::element_tree const& tree, layouted_element const& le, const si::input_state& input, const tg::aabb2& clip)
{
    CC_ASSERT(le.element->type == element_type::window);
    auto& c = tree.children_of(*le.element)[0];

    // bg
    add_quad(_render_data.lists.back(), clip, tg::color4(0, 0, 0, 0.3f), clip);
    add_quad(_render_data.lists.back(), shrink(clip, 1.f), tg::color4(0.8f, 0.8f, 1.0f, 0.9f), clip);

    // title bg
    {
        auto bb = tree.get_property(c, si::property::aabb);
        auto is_hover = input.hover_curr == c.id;
        auto is_pressed = input.pressed_curr == c.id;
        add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    }

    // title
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start + 1, le.child_start + le.child_count, input, clip);
}

void si::Simple2DMerger::render_child_range(si::element_tree const& tree, int range_start, int range_end, input_state const& input, tg::aabb2 const& clip)
{
    // NOTE: visibility is checked inside build_render_data
    for (auto i = range_start; i < range_end; ++i)
    {
        auto const& le = _layout_tree[i];
        build_render_data(tree, le, input, clip);
    }
}

tg::aabb2 si::Simple2DMerger::perform_child_layout_default(si::element_tree& tree, int parent_layout_idx, cc::span<si::element_tree_element> elements, float x, float y)
{
    /*
     * there is a child range allocated to parent_layout_idx (this is done in perform_layout)
     * this range has at most tree[parent_element].child_count slots
     * we reorder the children to get the actual z order (render order)
     * currently there are three types of children (in this order):
     * .. normal/abs elements (most frequent ones, declaration order)
     * .. windows (sorted by window idx)
     * .. detached (new roots, not part of parent's children, occupy last slots
     */

    auto win_start_offset = 8.f;
    auto has_windows = false;
    auto max_window_idx = 0;

    auto detached_cnt = 0;

    auto cy = y;
    auto maxx = x + 16;

    for (auto& c : elements)
    {
        // query positioning
        tg::pos2 abs_pos;
        si::placement placement;
        auto is_abs = tree.get_property_to(c, si::property::absolute_pos, abs_pos);
        auto is_detached = tree.get_property_or(c, si::property::detached, false);
        auto is_placed = tree.get_property_to(c, si::property::placement, placement);
        auto vis = tree.get_property_or(c, si::property::visibility, si::visibility::visible);

        if (vis == si::visibility::none)
            continue; // completely ignore

        // persistent window index handling
        if (c.type == element_type::window)
        {
            // first frame
            if (!is_abs)
            {
                tree.set_property(c, si::property::absolute_pos, {win_start_offset, win_start_offset});
                win_start_offset += 8.f;
            }

            // windows are layout later in this function
            has_windows = true;
        }
        // detached (tooltips, popups, etc.)
        else if (is_detached)
        {
            CC_ASSERT((is_abs || is_placed) && "detached elements must have absolute coordinates or placement");
            CC_ASSERT(parent_layout_idx >= 0 && "detached elements in ui root is currently not supported");

            ++detached_cnt;
            auto const& pe = _layout_tree[parent_layout_idx];
            auto cidx = pe.child_start + pe.element->children_count - detached_cnt;
            CC_ASSERT(_layout_tree[cidx].element == nullptr && "not cleaned properly?");

            // new layout root
            // NOTE: BEFORE recursing (for proper nested tooltips/popovers)
            _layout_detached_roots.push_back(cidx);

            if (is_placed)
            {
                // NOTE: BEFORE recursing (for proper nested tooltips/popovers)
                _deferred_placements.push_back({placement, parent_layout_idx, cidx});
                perform_layout(tree, c, cidx, x, cy);
            }
            else // absolute position
            {
                CC_ASSERT(is_abs);
                perform_layout(tree, c, cidx, abs_pos.x, abs_pos.y);
            }
        }
        // absolute positioning
        else if (is_abs)
        {
            CC_ASSERT(!is_placed && "absolute elements may not have placement");
            auto cidx = add_child_layout_element(parent_layout_idx);
            perform_layout(tree, c, cidx, x + abs_pos.x, y + abs_pos.y);
            // TODO: what about aabb? should it affect parent aabb or not?
        }
        // relative / top-down positioning
        else
        {
            CC_ASSERT(!is_placed && "relative elements may not have placement");
            auto cidx = add_child_layout_element(parent_layout_idx);
            auto bb = perform_layout(tree, c, cidx, x, cy);
            cy = bb.max.y + padding_child;
            maxx = tg::max(maxx, bb.max.x);
        }
    }

    // window sorting
    // NOTE: _tmp_windows can only be used here because recursive layouts would clear it again otherwise
    if (has_windows)
    {
        // TODO: focus window to top

        // collect windows with prev idx
        _tmp_windows.clear();
        for (auto& c : elements)
            if (c.type == element_type::window)
            {
                auto prev_c = _prev_ui->element_by_id(c.id);
                auto widx = _prev_ui->get_property_or(prev_c, si::property::detail::window_idx, max_window_idx + 1);
                max_window_idx = tg::max(widx, max_window_idx);
                _tmp_windows.push_back({&c, widx});
            }

        // sort windows
        std::sort(_tmp_windows.begin(), _tmp_windows.end());

        // perform layouting and set new indices
        for (auto i = 0; i < int(_tmp_windows.size()); ++i)
        {
            auto& c = *_tmp_windows[i].window;

            auto abs_pos = tree.get_property(c, si::property::absolute_pos);
            auto cidx = add_child_layout_element(parent_layout_idx);
            perform_layout(tree, c, cidx, x + abs_pos.x, y + abs_pos.y);

            tree.set_property(c, si::property::detail::window_idx, i);
        }
    }

    if (cy != y)
        cy -= padding_child; // remove last padding

    return tg::aabb2({x, y}, {maxx, cy});
}

void si::Simple2DMerger::resolve_deferred_placements(si::element_tree& tree)
{
    for (auto const& [placement, ref_idx, this_idx] : _deferred_placements)
    {
        // compute new pos based on placement
        auto pbb = ref_idx < 0 ? viewport : _layout_tree[ref_idx].bounds;
        auto cbb = _layout_tree[this_idx].bounds;
        auto p = placement.compute(pbb, mouse_pos, cbb);

        // ensure it doesn't leave the viewport
        p = tg::clamp(p, viewport.min, viewport.max - (cbb.max - cbb.min));

        // move sub-layout
        move_layout(tree, this_idx, p - cbb.min);
    }
}

int si::Simple2DMerger::add_child_layout_element(int parent_idx)
{
    if (parent_idx >= 0)
    {
        auto& p = _layout_tree[parent_idx];
        CC_ASSERT(p.element);
        CC_ASSERT(p.child_count < p.element->children_count && "cannot overalloc children");
        auto ci = p.child_start + p.child_count;
        ++p.child_count;
        return ci;
    }
    else // roots
    {
        _layout_roots.push_back(_layout_original_roots);
        return _layout_original_roots++;
    }
}

void si::Simple2DMerger::move_layout(si::element_tree& tree, int layout_idx, tg::vec2 delta)
{
    // TODO: simplify when switching to relative aabbs
    // TODO: simplify when caching text data in layouted

    auto& le = _layout_tree[layout_idx];
    CC_ASSERT(le.element);
    CC_ASSERT(le.child_count <= le.element->children_count);
    le.bounds.min += delta;
    le.bounds.max += delta;
    le.text_origin += delta;
    tree.set_property(*le.element, si::property::aabb, le.bounds);

    auto cs = le.child_start;
    auto ce = le.child_start + le.child_count; // ignores detached elements as le.child_count != le.element->child_count
    for (auto i = cs; i < ce; ++i)
        move_layout(tree, i, delta);
}

tg::aabb2 si::Simple2DMerger::perform_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y)
{
    // TODO: early out is possible

    // alloc space for children
    // children can be reordered and skipped but for now we don't support adding more
    auto const ccnt = e.children_count;
    auto const layout_child_start = int(_layout_tree.size());
    CC_ASSERT(_layout_tree.size() + ccnt <= _layout_tree.capacity() && "should be known a-priori");
    _layout_tree.resize(_layout_tree.size() + ccnt);

    // init layout element
    // NOTE: le stays valid because we reserved enough _layout_tree
    auto& le = _layout_tree[layout_idx];
    CC_ASSERT(le.element == nullptr && "not properly cleared?");
    CC_ASSERT(le.child_count == 0 && "not properly cleared?");
    le.element = &e;
    le.child_start = layout_child_start;
    le.no_input = tree.get_property_or(e, si::property::no_input, false);
    le.is_visible = tree.get_property_or(e, si::property::visibility, si::visibility::visible) == si::visibility::visible;

    // special types
    switch (e.type)
    {
    case element_type::checkbox:
        le.bounds = perform_checkbox_layout(tree, e, layout_idx, x, y);
        break;

    case element_type::slider:
        le.bounds = perform_slider_layout(tree, e, layout_idx, x, y);
        break;

    case element_type::window:
        le.bounds = perform_window_layout(tree, e, layout_idx, x, y);
        break;

    default:
        // generic handling for all other types
        float cx = x + padding_left;
        float cy = y + padding_top;

        auto maxx = cx;

        if (tree.has_property(e, si::property::text))
        {
            auto txt = tree.get_property(e, si::property::text);
            le.text_origin = {cx, cy};
            auto bb = get_text_bounds(txt, cx, cy);
            cy = bb.max.y;
            maxx = tg::max(maxx, bb.max.x);
            // TODO: padding for next child?
        }

        auto cbb = perform_child_layout_default(tree, layout_idx, tree.children_of(e), cx, cy);

        maxx = tg::max(maxx, cbb.max.x);
        cy = tg::max(cy, cbb.max.y);

        auto start = tg::pos2(x, y);
        auto end = tg::pos2(maxx + padding_right, cy + padding_bottom);
        le.bounds = tg::aabb2(start, end);
        break;
    }

    tree.set_property(e, si::property::aabb, le.bounds);
    return le.bounds;
}

void si::Simple2DMerger::render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip)
{
    // TODO: prebuild this in layout data
    auto const& e = *le.element;
    auto txt = tree.get_property(e, si::property::text);
    auto tp = le.text_origin;

    add_text_render_data(_render_data.lists.back(), txt, tp.x, tp.y, clip);
}

void si::Simple2DMerger::build_render_data(si::element_tree const& tree, layouted_element const& le, input_state const& input, tg::aabb2 clip)
{
    if (!le.is_visible)
        return; // hidden element

    CC_ASSERT(le.element);
    auto bb = le.bounds;
    auto bb_clip = intersection(bb, clip);
    if (!bb_clip.has_value())
        return; // early out: out of clip
    clip = bb_clip.value();
    auto etype = le.element->type;
    auto eid = le.element->id;

    // special handling
    switch (etype)
    {
    case element_type::checkbox:
        return render_checkbox(tree, le, input, clip);

    case element_type::slider:
        return render_slider(tree, le, input, clip);

    case element_type::window:
        return render_window(tree, le, input, clip);

        // generic handling
    default:
        auto has_bg = false;
        if (etype == element_type::button)
            has_bg = true;
        // TODO: more generic?

        if (has_bg)
        {
            auto is_hover = input.hover_curr == eid;
            auto is_pressed = input.pressed_curr == eid;
            add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
        }

        if (etype == element_type::tooltip || etype == element_type::popover)
        {
            add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 0, 0.8f), clip);
            add_quad(_render_data.lists.back(), shrink(bb, 1), tg::color4(0.9f, 0.9f, 1, 0.9f), clip);
        }

        if (tree.has_property(*le.element, si::property::text))
            render_text(tree, le, clip);

        // children
        render_child_range(tree, le.child_start, le.child_start + le.child_count, input, clip);
    }
}
