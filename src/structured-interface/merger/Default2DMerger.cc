#include "Default2DMerger.hh"

#include <chrono>

#include <rich-log/log.hh>

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

void si::Default2DMerger::emit_warning(si::element_handle id, cc::string_view msg)
{
    // TODO: customize
    LOG_WARN("[si] {} (element id {})", msg, id.id());
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

    if (le.no_input || le.style.visibility != style::visibility::visible)
        return nullptr; // does not receive input

    if (!contains(le.bounds(), p))
        return nullptr;

    auto cs = le.child_start;
    auto ce = le.child_start + le.child_count;
    for (auto i = ce - 1; i >= cs; --i)
        if (auto e = query_input_child_element_at(i, p))
            return e;

    if (!le.style.consumes_input)
        return nullptr;

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

    if (!contains(le.bounds(), p))
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
    _stylesheet.load_default_light_style();

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

    // step 0.5: root style
    auto t0 = std::chrono::high_resolution_clock::now();
    auto [ww, wh] = tg::size_of(viewport);
    auto root_style = _stylesheet.query_style(element_type::root, 0, {});
    root_style.font.size.resolve(_font.ref_size, _font.ref_size);
    root_style.bounds.width = ww;
    root_style.bounds.height = wh;
    root_style.bounds.left = 0;
    root_style.bounds.right = 0;
    root_style.bounds.top = 0;
    root_style.bounds.bottom = 0;
    root_style.positioning = style::positioning::absolute;

    // step 1: style
    {
        _layout_tree.reserve(ui.all_elements().size()); // might be reshuffled but is max size
        _layout_tree.resize(ui.roots().size());         // pre-alloc roots as there is no perform_layout for them

        compute_child_style(ui, -1, ui.roots(), root_style, input.hovers_last);
        CC_ASSERT(_layout_original_roots <= int(ui.roots().size()));

        // finalize roots
        // NOTE: must be here so detached > windows
        // TODO: maybe use a stable sort here?
        for (auto i : _layout_detached_roots)
            _layout_roots.push_back(i);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    _seconds_style = std::chrono::duration<double>(t1 - t0).count();

    // step 2: layout
    {
        // actual layouting
        resolve_layout_new(ui);

        // resolve constraints
        resolve_deferred_placements(ui);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    _seconds_layout = std::chrono::duration<double>(t2 - t1).count();

    // step 3: input
    {
        // TODO: layers, events?

        // mouse pos
        input.mouse_pos = mouse_pos;
        input.mouse_delta = mouse_pos - prev_mouse_pos;
        input.scroll_delta = scroll_delta;

        // update hover
        if (is_lmb_down) // mouse down? copy from last
        {
            input.direct_hover_curr = input.direct_hover_last;
            input.hovers_curr = input.hovers_last;
        }
        // otherwise search topmost
        else if (auto hc = query_input_element_at(mouse_pos))
        {
            input.direct_hover_curr = hc->id;

            // build hover stack
            while (hc)
            {
                input.hovers_curr.push_back(hc->id);
                hc = ui.parent_of(*hc);
            }
        }

        // update focus
        // TODO: can focus?
        input.focus_curr = input.focus_last;

        // clicks
        if (!is_lmb_down && was_lmb_down && !input.is_drag)
        {
            input.clicked_curr = input.direct_hover_last; // curr is already replaced
            if (input.focus_curr == input.clicked_curr)
                input.focus_curr = {}; // unfocus
            else
                input.focus_curr = input.clicked_curr;
        }

        // update pressed
        if (is_lmb_down)
        {
            input.pressed_curr = input.direct_hover_curr;
            drag_distance += length(input.mouse_delta);
            input.is_drag = drag_distance > 10; // configurable?
        }
        else
        {
            input.pressed_curr = {};
            drag_distance = 0;
            input.is_drag = false;
        }

        // focus windows
        if (!input.pressed_last.is_valid() && input.pressed_curr.is_valid())
        {
            auto e = ui.get_element_by_id(input.pressed_curr);
            while (e)
            {
                if (e->type == element_type::window)
                    ui.set_property(*e, si::property::detail::window_idx, tg::max<int>());
                e = ui.parent_of(*e);
            }
        }

        // write-through prev
        prev_mouse_pos = mouse_pos;
        was_lmb_down = is_lmb_down;

        // capture state
        uses_input = input.pressed_curr.is_valid() || input.direct_hover_curr.is_valid();
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    _seconds_input = std::chrono::duration<double>(t3 - t2).count();

    // step 4: render data
    {
        // TODO: reuse memory
        _render_data.lists.clear();
        _render_data.lists.emplace_back();

        // contains normal ui elements and detached ones
        for (auto i : _layout_roots)
            build_render_data(ui, _layout_tree[i], viewport);
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    _seconds_render_data = std::chrono::duration<double>(t4 - t3).count();

    // step 5: some cleanup
    _prev_ui = nullptr;
    _input = nullptr;

    return cc::move(ui);
}

void si::Default2DMerger::set_editable_text_glyphs(cc::string_view txt, float x, float y, const si::style::font& font)
{
    // TODO: reuse memory?
    cc::vector<merger::editable_text::glyph> glyphs;

    auto s = font.size.absolute / _font.ref_size;

    size_t idx = 0;
    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        auto const& g = _font.glyphs[c];
        glyphs.push_back({idx, 1, {{x, y}, {x + g.advance * s, y + _font.baseline_height * s}}});
        x += g.advance * s;

        ++idx;
    }

    _editable_text.set_glyphs(cc::move(glyphs));
}

tg::aabb2 si::Default2DMerger::get_text_bounds(cc::string_view txt, float x, float y, style::font const& font)
{
    auto s = font.size.absolute / _font.ref_size;
    auto ox = x;

    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        x += _font.glyphs[c].advance * s;
    }

    return {{ox, y}, {x, y + _font.baseline_height * s}};
}

void si::Default2DMerger::add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, style::font const& font, tg::aabb2 const&, size_t selection_start, size_t selection_count)
{
    auto s = font.size.absolute / _font.ref_size;

    // TODO: clip

    // baseline
    auto bx = x;
    auto by = y + _font.ascender * s;

    auto fc = font.color;
    auto sc = (fc.r + fc.g + fc.b) / 3 > 0.5f ? tg::color3::black : tg::color3::white;

    size_t idx = 0;
    for (auto c : txt)
    {
        if (c < ' ' || int(c) >= int(_font.glyphs.size()))
            c = '?';

        auto const& gi = _font.glyphs[c];

        if (gi.height > 0) // ignore invisible chars
        {
            auto pmin = tg::pos2(bx + gi.bearingX * s, by - gi.bearingY * s);
            auto pmax = pmin + tg::vec2(gi.width * s, gi.height * s);
            auto is_sel = selection_start <= idx && idx < selection_start + selection_count;
            add_quad_uv(rl, {pmin, pmax}, gi.uv, is_sel ? sc : fc);
        }

        bx += gi.advance * s;
        ++idx;
    }
}

void si::Default2DMerger::compute_style(si::element_tree& tree,
                                        si::element_tree_element& e,
                                        int layout_idx,
                                        int parent_layout_idx,
                                        si::StyleSheet::computed_style const& parent_style,
                                        int child_idx,
                                        int child_cnt,
                                        cc::span<element_handle> hover_stack)
{
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
    le.parent_idx = parent_layout_idx;

    // query style
    // NOTE: last input is used because curr is not yet assigned
    StyleSheet::style_key style_key;
    style_key.type = e.type;
    style_key.is_hovered = hover_stack.empty() ? false : hover_stack.back() == e.id;
    style_key.is_pressed = _input->pressed_last == e.id;
    style_key.is_focused = _input->focus_last == e.id;
    style_key.is_first_child = child_idx == 0;
    style_key.is_last_child = child_idx == child_cnt - 1;
    style_key.is_odd_child = child_idx % 2 == 0;
    style_key.is_checked = tree.get_property_or(e, si::property::state_u8, 0) >= 1;
    style_key.is_enabled = tree.get_property_or(e, si::property::enabled, true);
    style_key.style_class = tree.get_property_or(e, si::property::style_class, 0);
    le.style = _stylesheet.query_style(style_key, parent_style.hash, _style_stack_keys);

    // overwritable style
    le.style.visibility = tree.get_property_or(e, si::property::visibility, le.style.visibility);

    tg::pos2 abs_pos;
    if (tree.get_property_to(e, si::property::absolute_pos, abs_pos))
    {
        le.style.positioning = style::positioning::absolute;
        le.style.bounds.left = abs_pos.x;
        le.style.bounds.top = abs_pos.y;
    }

    tg::size2 fixed_size;
    if (tree.get_property_to(e, si::property::fixed_size, fixed_size))
    {
        le.style.bounds.width = fixed_size.width;
        le.style.bounds.height = fixed_size.height;
    }

    tree.get_property_each(e, si::property::style_value, [&](style::style_value const& v) {
        using style_entry = si::style::style_entry;
        switch (v.entry)
        {
        case style_entry::left_abs:
            le.style.bounds.left = v.value;
            break;
        case style_entry::left_rel:
            le.style.bounds.left.set_relative(v.value);
            break;
        case style_entry::top_abs:
            le.style.bounds.top = v.value;
            break;
        case style_entry::top_rel:
            le.style.bounds.top.set_relative(v.value);
            break;
        case style_entry::width_abs:
            le.style.bounds.width = v.value;
            break;
        case style_entry::width_rel:
            le.style.bounds.width.set_relative(v.value);
            break;
        case style_entry::height_abs:
            le.style.bounds.height = v.value;
            break;
        case style_entry::height_rel:
            le.style.bounds.height.set_relative(v.value);
            break;
        case style_entry::invalid:
            CC_UNREACHABLE("invalid style entry");
            break;
        }
    });

    // "resolve" style
    le.style.font.size.resolve(_font.ref_size, parent_style.font.size.absolute);
    CC_ASSERT(!le.style.border.left.has_percentage() && "not supported");
    CC_ASSERT(!le.style.border.right.has_percentage() && "not supported");
    CC_ASSERT(!le.style.border.top.has_percentage() && "not supported");
    CC_ASSERT(!le.style.border.bottom.has_percentage() && "not supported");
    le.style.border.left.resolve(0, 0);
    le.style.border.right.resolve(0, 0);
    le.style.border.top.resolve(0, 0);
    le.style.border.bottom.resolve(0, 0);

    // prepare text
    if (tree.has_property(e, si::property::text))
    {
        le.has_text = true;
        auto txt = tree.get_property(e, si::property::text);
        auto bb = get_text_bounds(txt, 0.f, 0.f, le.style.font);
        CC_ASSERT(bb.min.x == 0 && bb.min.y == 0); // otherwise something is fishy
        le.text_width = bb.max.x - bb.min.x;
        le.text_height = bb.max.y - bb.min.y;
    }

    // compute child style
    _style_stack_keys.push_back(style_key);
    compute_child_style(tree, layout_idx, tree.children_of(e), le.style, hover_stack.empty() ? hover_stack : hover_stack.first(hover_stack.size() - 1));
    _style_stack_keys.pop_back();
}

void si::Default2DMerger::compute_child_style(si::element_tree& tree,
                                              int parent_layout_idx,
                                              cc::span<si::element_tree_element> elements,
                                              si::StyleSheet::computed_style const& style,
                                              cc::span<element_handle> hover_stack)
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

    auto child_cnt = int(elements.size());
    si::placement placement;

    auto prev_normal_idx = -1;

    for (auto child_idx = 0; child_idx < int(elements.size()); ++child_idx)
    {
        auto& c = elements[child_idx];

        // query positioning
        auto is_detached = tree.get_property_or(c, si::property::detached, false);
        auto is_placed = tree.get_property_to(c, si::property::placement, placement);

        // persistent window index handling
        if (c.type == element_type::window)
        {
            // first frame
            if (!tree.has_property(c, si::property::absolute_pos))
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
            CC_ASSERT(parent_layout_idx >= 0 && "detached elements in ui root is currently not supported");

            // ignore vis none
            // NOTE: does not account for implicitly "none" yet
            if (tree.get_property_or(c, si::property::visibility, style::visibility::visible) != style::visibility::none)
            {
                ++detached_cnt;
                auto const& pe = _layout_tree[parent_layout_idx];
                auto cidx = pe.child_start + pe.element->children_count - detached_cnt;
                CC_ASSERT(_layout_tree[cidx].element == nullptr && "not cleaned properly?");

                // new layout root
                // NOTE: BEFORE recursing (for proper nested tooltips/popovers)
                _layout_detached_roots.push_back(cidx);

                // NOTE: BEFORE recursing (for proper nested tooltips/popovers)
                if (is_placed)
                    _deferred_placements.push_back({placement, parent_layout_idx, cidx});

                compute_style(tree, c, cidx, parent_layout_idx, style, 0, 0, hover_stack);
            }
        }
        else
        {
            CC_ASSERT(!is_placed && "normal elements may not have placement");
            auto cidx = add_child_layout_element(parent_layout_idx);
            compute_style(tree, c, cidx, parent_layout_idx, style, child_idx, child_cnt, hover_stack);

            // chain of "normal" siblings
            if (auto& le = _layout_tree[cidx]; le.is_normal())
            {
                le.prev_normal_idx = prev_normal_idx;
                prev_normal_idx = cidx;
            }
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

            auto cidx = add_child_layout_element(parent_layout_idx);
            compute_style(tree, c, cidx, parent_layout_idx, style, i, int(_tmp_windows.size()), hover_stack);

            tree.set_property(c, si::property::detail::window_idx, i);
        }
    }
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

void si::Default2DMerger::resolve_deferred_placements(si::element_tree& tree)
{
    // TODO: currently also computes placement for hidden elements
    //       it is not trivial to remove them though
    //       if this becomes a performance problem is can be done
    for (auto const& [placement, ref_idx, this_idx] : _deferred_placements)
    {
        // compute new pos based on placement
        auto pbb = ref_idx < 0 ? viewport : _layout_tree[ref_idx].bounds();
        auto cbb = _layout_tree[this_idx].bounds();
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
    le.x += delta.x;
    le.y += delta.y;
    le.text_origin += delta;
    le.content_x += delta.x;
    le.content_y += delta.y;
    tree.set_property(*le.element, si::property::aabb, le.bounds());

    auto cs = le.child_start;
    auto ce = le.child_start + le.child_count; // ignores detached elements as le.child_count != le.element->child_count
    for (auto i = cs; i < ce; ++i)
        move_layout(tree, i, delta);
}

void si::Default2DMerger::render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip, size_t selection_start, size_t selection_count)
{
    // TODO: prebuild this in layout data
    auto const& e = *le.element;
    auto txt = tree.get_property(e, si::property::text);
    auto tp = le.text_origin;

    add_text_render_data(_render_data.lists.back(), txt, tp.x, tp.y, le.style.font, clip, selection_start, selection_count);
}

void si::Default2DMerger::build_render_data(si::element_tree const& tree, layouted_element const& le, tg::aabb2 clip)
{
    if (le.style.visibility != style::visibility::visible)
        return; // hidden element

    CC_ASSERT(le.element);
    auto bb = le.bounds();
    auto bb_clip = intersection(bb, clip);
    if (!bb_clip.has_value())
        return; // early out: out of clip

    if (le.style.overflow == style::overflow::hidden)
        clip = bb_clip.value();

    tg::aabb2 text_cursor_bb;
    auto render_text_cursor = false;
    size_t text_sel_start = 0;
    size_t text_sel_count = 0;

    // generic draw cmds
    {
        auto& rl = _render_data.lists.back();

        auto const& border = le.style.border;
        auto border_left = border.left.get();
        auto border_right = border.right.get();
        auto border_top = border.top.get();
        auto border_bottom = border.bottom.get();

        // border (up to 4 quads)
        if (border_left > 0)
            add_quad(rl, {bb.min, {bb.min.x + border_left, bb.max.y}}, border.color, clip);
        if (border_right > 0)
            add_quad(rl, {{bb.max.x - border_right, bb.min.y}, bb.max}, border.color, clip);
        if (border_top > 0)
            add_quad(rl, {{bb.min.x + border_left, bb.min.y}, {bb.max.x - border_right, bb.min.y + border_top}}, border.color, clip);
        if (border_bottom > 0)
            add_quad(rl, {{bb.min.x + border_left, bb.max.y - border_bottom}, {bb.max.x - border_right, bb.max.y}}, border.color, clip);

        // background
        if (le.style.bg.color.a > 0)
        {
            auto bbg = bb;
            bbg.min.x += border_left;
            bbg.max.x -= border_right;
            bbg.min.y += border_top;
            bbg.max.y -= border_bottom;
            add_quad(rl, bbg, le.style.bg.color, clip);
        }

        // custom triangles
        auto tris = tree.get_property_or(*le.element, si::property::custom_triangles, {});
        for (auto const& v : tris)
        {
            rl.indices.push_back(int(rl.vertices.size()));
            rl.vertices.push_back({v.pos + tg::vec2(le.content_x, le.content_y), {0, 0}, to_rgba8(v.color)});
            rl.cmds.back().indices_count++;
        }

        // text edit selection and cursor
        if (le.is_in_text_edit)
        {
            auto const cursor_rad = 1;

            // TODO: multi line support
            auto smin = tg::pos2(tg::max<float>());
            auto smax = tg::pos2(tg::min<float>());
            auto any_sel = false;
            auto got_cursor = false;
            for (auto const& g : _editable_text.glyphs())
            {
                if (g.start <= _editable_text.cursor() && _editable_text.cursor() < g.start + g.count)
                {
                    text_cursor_bb = g.bounds;
                    text_cursor_bb.max.x = text_cursor_bb.min.x + cursor_rad;
                    text_cursor_bb.min.x = text_cursor_bb.min.x - cursor_rad;
                    got_cursor = true;
                }

                if (g.start + g.count <= _editable_text.selection_start())
                    continue;
                if (g.start >= _editable_text.selection_start() + _editable_text.selection_count())
                    continue;

                smin = min(smin, g.bounds.min);
                smax = max(smax, g.bounds.max);
                any_sel = true;
            }

            if (!got_cursor)
            {
                if (_editable_text.glyphs().empty()) // not text at all
                {
                    text_cursor_bb.min = bb.min - tg::vec2(cursor_rad, 0);
                    text_cursor_bb.max = bb.min + tg::vec2(cursor_rad, _font.baseline_height * le.style.font.size.absolute / _font.ref_size);
                }
                else // after last text
                {
                    text_cursor_bb = _editable_text.glyphs().back().bounds;
                    text_cursor_bb.min.x = text_cursor_bb.max.x - cursor_rad;
                    text_cursor_bb.max.x = text_cursor_bb.max.x + cursor_rad;
                }
            }

            if (any_sel) // render selection
            {
                // TODO: style sheet?
                auto fc = le.style.font.color;
                auto avg = (fc.r + fc.g + fc.b) / 3;
                auto sc = avg > 0.5 ? tg::color3::white : tg::color3::black;
                add_quad(rl, {smin, smax}, sc, clip);
            }

            // NOTE: cursor is rendered _after_ text
            render_text_cursor = int(total_time * 2) % 2 == 0;
            text_sel_start = _editable_text.selection_start();
            text_sel_count = _editable_text.selection_count();
        }
    }

    // text
    // TODO: invert color for selection?
    if (tree.has_property(*le.element, si::property::text))
        render_text(tree, le, clip, text_sel_start, text_sel_count);

    // text cursor
    if (render_text_cursor)
        add_quad(_render_data.lists.back(), text_cursor_bb, le.style.font.color, clip);

    // children
    render_child_range(tree, le.child_start, le.child_start + le.child_count, clip);
}
