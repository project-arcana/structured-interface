#include "Default2DMerger.hh"

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

void add_quad(si::Default2DMerger::render_list& rl, tg::aabb2 bb, tg::color4 color, tg::aabb2 const& clip)
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
void add_quad_uv(si::Default2DMerger::render_list& rl, tg::aabb2 const& bb, tg::aabb2 const& uv, tg::color4 color)
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

si::element_tree_element* si::Default2DMerger::query_input_element_at(tg::pos2 p) const
{
    for (auto i = int(_layout_roots.size()) - 1; i >= 0; --i)
        if (auto e = query_input_child_element_at(_layout_roots[i], p))
            return e;
    return nullptr;
}

si::element_tree_element* si::Default2DMerger::query_input_child_element_at(int layout_idx, tg::pos2 p) const
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

si::Default2DMerger::layouted_element const* si::Default2DMerger::query_layout_element_at(tg::pos2 p) const
{
    for (auto i = int(_layout_roots.size()) - 1; i >= 0; --i)
        if (auto e = query_child_layout_element_at(_layout_roots[i], p))
            return e;
    return nullptr;
}

si::Default2DMerger::layouted_element const* si::Default2DMerger::query_child_layout_element_at(int layout_idx, tg::pos2 p) const
{
    auto const& le = _layout_tree[layout_idx];

    if (!contains(le.bounds, p))
        return nullptr; // TODO: proper handling of extended children (like in context menus)

    auto cs = le.child_start;
    auto ce = le.child_start + le.child_count;
    for (auto i = ce - 1; i >= cs; --i)
        if (auto e = query_child_layout_element_at(i, p))
            return e;

    return &le;
}

si::Default2DMerger::Default2DMerger()
{
    // TODO: configurable style
    // TODO: configurable font
    load_default_font();
}

si::element_tree si::Default2DMerger::operator()(si::element_tree const& prev_ui, si::element_tree&& ui, input_state& input)
{
    // step 0: prepare data
    _prev_ui = &prev_ui;
    _input = &input;
    _layout_tree.clear();
    _layout_roots.clear();
    _layout_detached_roots.clear();
    _layout_original_roots = 0;
    _deferred_placements.clear();
    _style_stack_keys.clear();
    _is_in_text_edit = false;

    // step 1: layout
    {
        auto root_style = _style_cache.query_style(element_type::root, 0, {});
        auto rx = root_style.margin.left + root_style.border.left + root_style.padding.left;
        auto ry = root_style.margin.top + root_style.border.top + root_style.padding.top;

        _layout_tree.reserve(ui.all_elements().size()); // might be reshuffled but is max size
        _layout_tree.resize(ui.roots().size());         // pre-alloc roots as there is not perform_layout for them

        // actual layouting
        perform_child_layout_default(ui, -1, ui.roots(), rx, ry, root_style);
        CC_ASSERT(_layout_original_roots <= int(ui.roots().size()));

        // finalize roots
        // NOTE: must be here so detached > windows
        // TODO: maybe use a stable sort here?
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

        // update hover
        if (is_lmb_down) // mouse down? copy from last
            input.hover_curr = input.hover_last;
        // otherwise search topmost
        else if (auto hc = query_input_element_at(mouse_pos))
            input.hover_curr = hc->id;

        // update focus
        // TODO: can focus?
        input.focus_curr = input.focus_last;

        // clicks
        if (is_lmb_down && !was_lmb_down && !input.is_drag)
        {
            input.clicked_curr = input.hover_last; // curr is already replaced
            if (input.focus_curr == input.clicked_curr)
                input.focus_curr = {}; // unfocus
            else
                input.focus_curr = input.clicked_curr;
        }

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
            input.is_drag = false;
        }

        // write-through prev
        prev_mouse_pos = mouse_pos;
        was_lmb_down = is_lmb_down;
    }

    // step 3: render data
    {
        // TODO: reuse memory
        _render_data.lists.clear();
        _render_data.lists.emplace_back();

        // contains normal ui elements and detached ones
        for (auto i : _layout_roots)
            build_render_data(ui, _layout_tree[i], viewport);
    }

    // step 4: some cleanup
    _prev_ui = nullptr;
    _input = nullptr;

    return cc::move(ui);
}

tg::aabb2 si::Default2DMerger::get_text_bounds(cc::string_view txt, float x, float y, style::font const& font)
{
    auto s = font.size / _font.ref_size;
    auto ox = x;

    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        x += _font.glyphs[c].advance * s;
    }

    return {{ox, y}, {x, y + _font.baseline_height * s}};
}

void si::Default2DMerger::add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, style::font const& font, tg::aabb2 const&)
{
    auto s = font.size / _font.ref_size;

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
            add_quad_uv(rl, {pmin, pmax}, gi.uv, font.color);
        }

        bx += gi.advance * s;
    }
}

tg::pos2 si::Default2DMerger::perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style)
{
    CC_ASSERT(e.type == element_type::checkbox);

    auto txt = tree.get_property(e, si::property::text);

    auto bs = tg::round(style.font.size * 1.1f);

    auto tx = x + bs + 4.0f; // TODO: how to configure?
    _layout_tree[layout_idx].text_origin = {tx, y};
    auto tbb = get_text_bounds(txt, tx, y, style.font);

    // children (e.g. tooltips)
    perform_child_layout_default(tree, layout_idx, tree.children_of(e), x, y, style);

    return tbb.max;
}

void si::Default2DMerger::render_checkbox(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip)
{
    CC_ASSERT(le.element->type == element_type::checkbox);

    auto eid = le.element->id;
    auto bs = tg::round(le.font.size * 1.1f);
    auto is_hover = _input->hover_curr == eid;
    auto is_pressed = _input->pressed_curr == eid;
    auto is_checked = tree.get_property(*le.element, si::property::state_u8) == uint8_t(true);

    // checkbox
    // TODO: configure via stylesheet
    auto cbb = tg::aabb2(le.content_start, tg::size2(bs));
    add_quad(_render_data.lists.back(), cbb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    if (is_checked)
        add_quad(_render_data.lists.back(), shrink(cbb, tg::round(bs * 0.2f)), tg::color4(0, 0, 0, 0.6f), clip);

    // checkbox text
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start, le.child_start + le.child_count, clip);
}

tg::pos2 si::Default2DMerger::perform_slider_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style)
{
    CC_ASSERT(e.type == element_type::slider);
    CC_ASSERT(e.children_count >= 1 && "expected at least one child");
    auto& c = tree.children_of(e)[0];
    CC_ASSERT(c.type == element_type::slider_area);

    auto& cle = _layout_tree[add_child_layout_element(layout_idx)];
    cle.element = &c;

    auto txt = tree.get_property(e, si::property::text);

    auto slider_width = 140.f;
    auto slider_height = style.font.size * 1.2f;
    cle.bounds = tg::aabb2({x, y}, tg::size2(slider_width, slider_height));
    tree.set_property(c, si::property::aabb, cle.bounds);

    // value text
    {
        // TODO: clip
        auto txt = tree.get_property(c, si::property::text);
        auto tbb = get_text_bounds(txt, x, y, style.font);
        auto tx = x + (slider_width - (tbb.max.x - tbb.min.x)) / 2;
        cle.text_origin = {tx, y};
    }

    // slider text
    auto tx = x + slider_width + 4.0f; // TODO: style sheet?
    _layout_tree[layout_idx].text_origin = {tx, y};
    auto tbb = get_text_bounds(txt, tx, y, style.font);

    // children (e.g. tooltips)
    perform_child_layout_default(tree, layout_idx, tree.children_of(e).subspan(1), x, y, style);

    return tbb.max;
}

void si::Default2DMerger::render_slider(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip)
{
    CC_ASSERT(le.element->type == element_type::slider);
    auto& c = tree.children_of(*le.element)[0];

    // TODO: style sheet
    auto is_hover = _input->hover_curr == c.id;
    auto is_pressed = _input->pressed_curr == c.id;

    // slider box
    auto sbb = tree.get_property(c, si::property::aabb);
    add_quad(_render_data.lists.back(), sbb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);

    // slider knob
    {
        auto khw = 7; // TODO: stylesheet
        auto t = tree.get_property(c, si::property::state_f32);
        auto x = tg::mix(sbb.min.x + khw + 1, sbb.max.x - khw - 1, t);
        add_quad(_render_data.lists.back(), tg::aabb2({x - khw, sbb.min.y + 1}, {x + khw, sbb.max.y - 1}), tg::color4(1, 1, 1, 0.4f), clip);
    }

    // value text
    render_text(tree, _layout_tree[le.child_start], clip);

    // slider text
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start + 1, le.child_start + le.child_count, clip);
}

tg::pos2 si::Default2DMerger::perform_window_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style)
{
    CC_ASSERT(e.type == element_type::window);
    CC_ASSERT(e.children_count >= 1 && "expected at least one child");
    auto& c = tree.children_of(e)[0];
    CC_ASSERT(c.type == element_type::clickable_area);

    auto& cle = _layout_tree[add_child_layout_element(layout_idx)];
    cle.element = &c;

    // title
    // TODO: completely via stylesheet?
    auto txt = tree.get_property(e, si::property::text);
    _layout_tree[layout_idx].text_origin = {x, y};
    auto tbb = get_text_bounds(txt, x, y, style.font);

    // content
    auto wy = tbb.max.y + 8.f; // TODO: style sheet
    auto cbb = perform_child_layout_default(tree, layout_idx, tree.children_of(e).subspan(1), x, wy, style);

    // bb
    auto maxx = tg::max(tbb.max.x, cbb.max.x);
    auto maxy = cbb.max.y;

    // clickable area
    // TODO: no margin?
    cle.bounds = {{x, y}, {maxx, tbb.max.y + 4.f}}; // TODO: style sheet
    tree.set_property(c, si::property::aabb, cle.bounds);

    return {maxx, maxy};
}

void si::Default2DMerger::render_window(si::element_tree const& tree, layouted_element const& le, const tg::aabb2& clip)
{
    CC_ASSERT(le.element->type == element_type::window);
    auto& c = tree.children_of(*le.element)[0];

    // title bg
    // TODO: via stylesheet?
    {
        auto bb = tree.get_property(c, si::property::aabb);
        auto is_hover = _input->hover_curr == c.id;
        auto is_pressed = _input->pressed_curr == c.id;
        add_quad(_render_data.lists.back(), bb, tg::color4(0, 0, 1, is_pressed ? 0.5f : is_hover ? 0.3f : 0.2f), clip);
    }

    // title
    render_text(tree, le, clip);

    // children
    render_child_range(tree, le.child_start + 1, le.child_start + le.child_count, clip);
}

void si::Default2DMerger::render_child_range(si::element_tree const& tree, int range_start, int range_end, tg::aabb2 const& clip)
{
    // NOTE: visibility is checked inside build_render_data
    for (auto i = range_start; i < range_end; ++i)
    {
        auto const& le = _layout_tree[i];
        build_render_data(tree, le, clip);
    }
}

void si::Default2DMerger::text_edit_add_char(char c)
{
    CC_ASSERT(is_in_text_edit());
    _editable_text += c;
}

void si::Default2DMerger::text_edit_backspace()
{
    CC_ASSERT(is_in_text_edit());
    if (_editable_text.size() > 0)
        _editable_text.pop_back();
}

void si::Default2DMerger::text_edit_entf()
{
    CC_ASSERT(is_in_text_edit());
    if (_editable_text.size() > 0)
        _editable_text.pop_back();
}

tg::aabb2 si::Default2DMerger::perform_child_layout_default(si::element_tree& tree, //
                                                            int parent_layout_idx,
                                                            cc::span<si::element_tree_element> elements,
                                                            float x,
                                                            float y,
                                                            StyleSheet::computed_style const& style)
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

    auto cx = x;
    auto cy = y;
    auto maxx = x + 16;
    auto maxy = y;

    auto child_idx = 0;
    auto child_cnt = int(elements.size());

    style::margin collapsible_margin;
    switch (style.layout)
    {
    case si::style::layout::top_down:
        collapsible_margin.top = style.margin.top;
        break;
    case si::style::layout::left_right:
        collapsible_margin.left = style.margin.left;
        break;
    default:
        CC_UNREACHABLE("not implemented");
    }

    for (auto& c : elements)
    {
        // query positioning
        tg::pos2 abs_pos;
        si::placement placement;
        auto is_abs = tree.get_property_to(c, si::property::absolute_pos, abs_pos);
        auto is_detached = tree.get_property_or(c, si::property::detached, false);
        auto is_placed = tree.get_property_to(c, si::property::placement, placement);
        auto vis = tree.get_property_or(c, si::property::visibility, si::style::visibility::visible);

        if (vis == si::style::visibility::none)
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
                perform_layout(tree, c, cidx, cx, cy, style, 0, 0, {});
            }
            else // absolute position
            {
                CC_ASSERT(is_abs);
                perform_layout(tree, c, cidx, abs_pos.x, abs_pos.y, style, 0, 0, {});
            }
        }
        // absolute positioning
        else if (is_abs)
        {
            CC_ASSERT(!is_placed && "absolute elements may not have placement");
            auto cidx = add_child_layout_element(parent_layout_idx);
            perform_layout(tree, c, cidx, x + abs_pos.x, y + abs_pos.y, style, child_idx, child_cnt, {});
            // TODO: what about aabb? should it affect parent aabb or not?
        }
        // normal, relative / top-down positioning
        else
        {
            CC_ASSERT(!is_placed && "relative elements may not have placement");
            auto cidx = add_child_layout_element(parent_layout_idx);
            auto [bb, cm] = perform_layout(tree, c, cidx, cx, cy, style, child_idx, child_cnt, collapsible_margin);
            maxx = tg::max(maxx, bb.max.x);
            maxy = tg::max(maxy, bb.max.y);

            switch (style.layout)
            {
            case style::layout::top_down:
                cy = bb.max.y + cm.bottom;
                collapsible_margin.top = cm.bottom;
                break;
            case style::layout::left_right:
                cx = bb.max.x + cm.right;
                collapsible_margin.left = cm.right;
                break;
            }
        }

        ++child_idx;
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
                auto prev_c = _prev_ui->get_element_by_id(c.id);
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
            perform_layout(tree, c, cidx, x + abs_pos.x, y + abs_pos.y, style, 0, 0, {});

            tree.set_property(c, si::property::detail::window_idx, i);
        }
    }

    return tg::aabb2({x, y}, {maxx, maxy});
}

void si::Default2DMerger::resolve_deferred_placements(si::element_tree& tree)
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

int si::Default2DMerger::add_child_layout_element(int parent_idx)
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

void si::Default2DMerger::move_layout(si::element_tree& tree, int layout_idx, tg::vec2 delta)
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

cc::pair<tg::aabb2, si::style::margin> si::Default2DMerger::perform_layout(si::element_tree& tree,
                                                                           si::element_tree_element& e,
                                                                           int layout_idx,
                                                                           float x,
                                                                           float y,
                                                                           StyleSheet::computed_style const& parent_style,
                                                                           int child_idx,
                                                                           int child_cnt,
                                                                           style::margin const& collapsible_margin)
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
    le.is_visible = tree.get_property_or(e, si::property::visibility, si::style::visibility::visible) == si::style::visibility::visible;

    // query style
    // NOTE: last input is used because curr is not yet assigned
    StyleSheet::style_key style_key;
    style_key.type = e.type;
    style_key.is_hovered = _input->hover_last == e.id;
    style_key.is_pressed = _input->pressed_last == e.id;
    style_key.is_focused = _input->focus_last == e.id;
    style_key.is_first_child = child_idx == 0;
    style_key.is_last_child = child_idx == child_cnt - 1;
    style_key.is_odd_child = child_idx % 2 == 0;
    style_key.style_class = 0; // TODO
    auto const style = _style_cache.query_style(style_key, parent_style.hash, _style_stack_keys);
    _style_stack_keys.push_back(style_key);
    le.bg = style.bg;
    le.font = style.font;
    le.border = style.border;

    // apply margin
    // TODO: from-right and from-bottom modes?
    x += tg::max(0, style.margin.left - collapsible_margin.left);
    y += tg::max(0, style.margin.top - collapsible_margin.top);

    // apply border and padding
    auto cx = x + style.border.left + style.padding.left;
    auto cy = y + style.border.top + style.padding.top;
    le.content_start = {cx, cy};

    // text edit
    if (e.type == element_type::input && tree.get_property_or(e, si::property::edit_text, false))
    {
        _is_in_text_edit = true;
        auto txt = tree.get_property(e, si::property::text);

        // set editable text on first edit
        auto pe = _prev_ui->get_element_by_id(e.id);
        auto prev_edit = _prev_ui->get_property_or(pe, si::property::edit_text, false);
        if (!prev_edit)
            _editable_text = txt;

        // sync text property with editable text
        if (_editable_text != txt)
            tree.set_property(e, si::property::text, _editable_text);
    }

    auto cmax = tg::pos2(cx, cy);
    switch (e.type) // special types
    {
    case element_type::checkbox:
        cmax = perform_checkbox_layout(tree, e, layout_idx, cx, cy, style);
        break;

    case element_type::slider:
        cmax = perform_slider_layout(tree, e, layout_idx, cx, cy, style);
        break;

    case element_type::window:
        cmax = perform_window_layout(tree, e, layout_idx, cx, cy, style);
        break;

    default:
        // generic handling for all other types
        if (tree.has_property(e, si::property::text))
        {
            auto txt = tree.get_property(e, si::property::text);
            le.text_origin = {cx, cy};
            auto bb = get_text_bounds(txt, cx, cy, style.font);
            cy = bb.max.y;
            cmax = tg::max(cmax, bb.max);
            // TODO: padding for next child?
        }

        auto cbb = perform_child_layout_default(tree, layout_idx, tree.children_of(e), cx, cy, style);

        // fixed size
        // TODO: via style sheet
        tg::size2 fs;
        if (tree.get_property_to(e, si::property::fixed_size, fs))
        {
            cmax.x = x + fs.width;
            cmax.y = y + fs.height;
        }

        cmax = tg::max(cmax, cbb.max);
        break;
    }

    // pop style
    _style_stack_keys.pop_back();

    // finalize bounds
    le.bounds = tg::aabb2({x, y}, cmax + tg::vec2(style.padding.right + style.border.right, style.padding.bottom + style.border.bottom));
    tree.set_property(e, si::property::aabb, le.bounds);
    return {le.bounds, style.margin};
}

void si::Default2DMerger::render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip)
{
    // TODO: prebuild this in layout data
    auto const& e = *le.element;
    auto txt = tree.get_property(e, si::property::text);
    auto tp = le.text_origin;

    add_text_render_data(_render_data.lists.back(), txt, tp.x, tp.y, le.font, clip);
}

void si::Default2DMerger::build_render_data(si::element_tree const& tree, layouted_element const& le, tg::aabb2 clip)
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

    // generic draw cmds
    {
        auto& rl = _render_data.lists.back();

        // border (up to 4 quads)
        if (le.border.left > 0)
            add_quad(rl, {bb.min, {bb.min.x + le.border.left, bb.max.y}}, le.border.color, clip);
        if (le.border.right > 0)
            add_quad(rl, {{bb.max.x - le.border.right, bb.min.y}, bb.max}, le.border.color, clip);
        if (le.border.top > 0)
            add_quad(rl, {{bb.min.x + le.border.left, bb.min.y}, {bb.max.x - le.border.right, bb.min.y + le.border.top}}, le.border.color, clip);
        if (le.border.bottom > 0)
            add_quad(rl, {{bb.min.x + le.border.left, bb.max.y - le.border.bottom}, {bb.max.x - le.border.right, bb.max.y}}, le.border.color, clip);

        // background
        if (le.bg.color.a > 0)
        {
            auto bbg = bb;
            bbg.min.x += le.border.left;
            bbg.max.x -= le.border.right;
            bbg.min.y += le.border.top;
            bbg.max.y -= le.border.bottom;
            add_quad(rl, bbg, le.bg.color, clip);
        }

        // custom triangles
        auto tris = tree.get_property_or(*le.element, si::property::custom_triangles, {});
        for (auto const& v : tris)
        {
            rl.indices.push_back(int(rl.vertices.size()));
            rl.vertices.push_back({v.pos + le.content_start, {0, 0}, to_rgba8(v.color)});
            rl.cmds.back().indices_count++;
        }
    }

    // special handling
    switch (etype)
    {
    case element_type::checkbox:
        return render_checkbox(tree, le, clip);

    case element_type::slider:
        return render_slider(tree, le, clip);

    case element_type::window:
        return render_window(tree, le, clip);

        // generic handling
    default:
        // text
        if (tree.has_property(*le.element, si::property::text))
            render_text(tree, le, clip);

        // children
        render_child_range(tree, le.child_start, le.child_start + le.child_count, clip);
    }
}
