#include "Default2DMerger.hh"

#include <structured-interface/element_tree.hh>
#include <structured-interface/properties.hh>

// TODO: also resolve deferred placement here
// TODO: root style (margin, padding, etc.)
// TODO: margin.left and bounds.left is kinda redundant?
// TODO: support min/max width/height
// TODO: use bounds right/bottom

namespace
{
// on_rel is either the relative value it references OR a function to compute it
template <class AutoF, class RelF>
float resolve(si::style::value& v, AutoF&& on_auto, RelF&& on_rel)
{
    if (v.is_resolved())
        return v.absolute;

    if (v.is_auto())
    {
        if constexpr (std::is_arithmetic_v<std::remove_reference_t<AutoF>>)
        {
            v = float(on_auto);
            return v.absolute;
        }
        else
        {
            v = float(on_auto());
            return v.absolute;
        }
    }

    CC_ASSERT(v.has_percentage()); // otherwise would have been resolved

    float rel_ref;
    if constexpr (std::is_arithmetic_v<std::remove_reference_t<RelF>>)
        rel_ref = float(on_rel);
    else
        rel_ref = float(on_rel());

    v.absolute += rel_ref * v.relative;
    v.relative = 0;
    return v.absolute;
}
}

void si::Default2DMerger::resolve_layout_new(si::element_tree& tree)
{
    struct layouter_t
    {
        si::element_tree& tree;
        Default2DMerger& merger;

        layouter_t(si::element_tree& t, Default2DMerger& m) : tree(t), merger(m) {}

        // helper
    public:
        bool is_valid(float v) const { return v != computing && v != unassigned && tg::is_finite(v); }

        layouted_element& parent_of(layouted_element& e)
        {
            CC_ASSERT(!e.is_root());
            return merger._layout_tree[e.parent_idx];
        }

        float reference_parent_x(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.min.x;

            auto& p = parent_of(e);

            switch (p.style.box_child_ref)
            {
            case style::box_type::content_box:
                return compute_content_x(p);

            case style::box_type::padding_box:
                return compute_x(p) + p.style.border.left.get();

            case style::box_type::border_box:
                return compute_x(p);
            }

            CC_UNREACHABLE("invalid box type");
        }

        float reference_parent_y(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.min.y;

            auto& p = parent_of(e);

            switch (p.style.box_child_ref)
            {
            case style::box_type::content_box:
                return compute_content_y(p);

            case style::box_type::padding_box:
                return compute_y(p) + p.style.border.top.get();

            case style::box_type::border_box:
                return compute_y(p);
            }

            CC_UNREACHABLE("invalid box type");
        }

        float reference_parent_content_x_with_text(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.min.x;

            auto& p = parent_of(e);

            auto x = compute_content_x(p);
            if (p.style.layout == style::layout::left_right)
                x += p.text_width; // padding after text?
            return x;
        }

        float reference_parent_content_y_with_text(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.min.y;

            auto& p = parent_of(e);

            auto y = compute_content_y(p);
            if (p.style.layout == style::layout::top_down)
                y += p.text_height; // padding after text?
            return y;
        }

        float reference_parent_width(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.max.x - merger.viewport.min.x;

            auto& p = parent_of(e);

            switch (e.style.box_child_ref)
            {
            case style::box_type::content_box:
                return compute_content_width(p);

            case style::box_type::padding_box:
                return compute_width(p) - p.style.border.left.get() - p.style.border.right.get();

            case style::box_type::border_box:
                return compute_width(p);
            }

            CC_UNREACHABLE("invalid box type");
        }

        float reference_parent_height(layouted_element& e)
        {
            if (e.is_root())
                return merger.viewport.max.y - merger.viewport.min.y;

            auto& p = parent_of(e);

            switch (e.style.box_child_ref)
            {
            case style::box_type::content_box:
                return compute_content_height(p);

            case style::box_type::padding_box:
                return compute_height(p) - p.style.border.top.get() - p.style.border.bottom.get();

            case style::box_type::border_box:
                return compute_height(p);
            }

            CC_UNREACHABLE("invalid box type");
        }

        bool is_parent_left_right(layouted_element& e) { return !e.is_root() && parent_of(e).is_left_right_layout(); }
        bool is_parent_top_down(layouted_element& e) { return e.is_root() || parent_of(e).is_top_down_layout(); }

        float get_right(layouted_element& e) { return compute_x(e) + compute_width(e); }
        float get_bottom(layouted_element& e) { return compute_y(e) + compute_height(e); }
        float get_content_right(layouted_element& e) { return compute_content_x(e) + compute_content_width(e); }

        float get_left_margin(layouted_element& e)
        {
            return resolve(e.style.margin.left, 0.f, [&] { return reference_parent_width(e); });
        }
        float get_right_margin(layouted_element& e)
        {
            return resolve(e.style.margin.right, 0.f, [&] { return reference_parent_width(e); });
        }
        float get_top_margin(layouted_element& e)
        {
            return resolve(e.style.margin.top, 0.f, [&] { return reference_parent_height(e); });
        }
        float get_bottom_margin(layouted_element& e)
        {
            return resolve(e.style.margin.bottom, 0.f, [&] { return reference_parent_height(e); });
        }

        // main layout computations
    public:
        float compute_x(layouted_element& e)
        {
            CC_ASSERT(e.x != computing && "cyclic layout detected");
            if (e.x != unassigned)
                return e.x;
            e.x = computing;

            if (e.is_absolute())
            {
                auto is_left_auto = e.style.bounds.left.is_auto();
                auto is_right_auto = e.style.bounds.right.is_auto();
                auto is_width_auto = e.style.bounds.width.is_auto();

                if (is_left_auto && is_right_auto && !is_width_auto)
                {
                    CC_ASSERT(false && "TODO: centering");
                }
                else if (!is_left_auto) // left aligned
                {
                    auto margin_left = resolve(e.style.margin.left, 0.f, [&] { return reference_parent_width(e); });
                    auto left = resolve(e.style.bounds.left, 0.f, [&] { return reference_parent_width(e); });
                    e.x = reference_parent_x(e) + margin_left + left;
                }
                else // right aligned
                {
                    auto margin_right = resolve(e.style.margin.right, 0.f, [&] { return reference_parent_width(e); });
                    auto right = resolve(e.style.bounds.right, 0.f, [&] { return reference_parent_width(e); });
                    e.x = reference_parent_x(e) + reference_parent_width(e) - margin_right - right - compute_width(e);
                }
            }
            else if (e.has_prev_normal_sibling() && is_parent_left_right(e)) // left-right layout with prev sibling
            {
                auto margin_left = resolve(e.style.margin.left, 0.f, [&] { return reference_parent_width(e); });
                auto& sibling = merger._layout_tree[e.prev_normal_idx];
                e.x = get_right(sibling) + tg::max(margin_left, get_right_margin(sibling));
            }
            else
            {
                auto margin_left = resolve(e.style.margin.left, 0.f, [&] { return reference_parent_width(e); });
                e.x = reference_parent_content_x_with_text(e) + margin_left;
            }

            CC_ASSERT(is_valid(e.x));
            return e.x;
        }

        float compute_y(layouted_element& e)
        {
            CC_ASSERT(e.y != computing && "cyclic layout detected");
            if (e.y != unassigned)
                return e.y;
            e.y = computing;

            if (e.is_absolute())
            {
                auto is_top_auto = e.style.bounds.top.is_auto();
                auto is_bottom_auto = e.style.bounds.bottom.is_auto();
                auto is_height_auto = e.style.bounds.height.is_auto();

                if (is_top_auto && is_bottom_auto && !is_height_auto)
                {
                    CC_ASSERT(false && "TODO: centering");
                }
                else if (!is_top_auto) // top aligned
                {
                    auto margin_top = resolve(e.style.margin.top, 0.f, [&] { return reference_parent_height(e); });
                    auto top = resolve(e.style.bounds.top, 0.f, [&] { return reference_parent_height(e); });
                    e.y = reference_parent_y(e) + margin_top + top;
                }
                else // bottom aligned
                {
                    auto margin_bottom = resolve(e.style.margin.bottom, 0.f, [&] { return reference_parent_height(e); });
                    auto bottom = resolve(e.style.bounds.bottom, 0.f, [&] { return reference_parent_height(e); });
                    e.y = reference_parent_y(e) + reference_parent_height(e) - margin_bottom - bottom - compute_height(e);
                }
            }
            else if (e.has_prev_normal_sibling() && is_parent_top_down(e)) // top-down layout with prev sibling
            {
                auto margin_top = resolve(e.style.margin.top, 0.f, [&] { return reference_parent_height(e); });
                auto& sibling = merger._layout_tree[e.prev_normal_idx];
                e.y = get_bottom(sibling) + tg::max(margin_top, get_bottom_margin(sibling));
            }
            else
            {
                auto margin_top = resolve(e.style.margin.top, 0.f, [&] { return reference_parent_height(e); });
                e.y = reference_parent_content_y_with_text(e) + margin_top;
            }

            CC_ASSERT(is_valid(e.y));
            return e.y;
        }

        float compute_width(layouted_element& e)
        {
            CC_ASSERT(e.width != computing && "cyclic layout detected");
            if (e.width != unassigned)
                return e.width;
            e.width = computing;

            float w = 0.f;

            // stretched
            if (e.is_absolute() && e.style.bounds.width.is_auto() && !e.style.bounds.left.is_auto() && !e.style.bounds.right.is_auto())
            {
                auto x = compute_x(e);

                auto margin_right = resolve(e.style.margin.right, 0.f, [&] { return reference_parent_width(e); });
                auto right = resolve(e.style.bounds.right, 0.f, [&] { return reference_parent_width(e); });
                auto max_x = reference_parent_x(e) + reference_parent_width(e) - margin_right - right;

                w = max_x - x;
            }
            else if (e.style.box_sizing == style::box_type::content_box || e.style.bounds.width.is_auto()) // from content
            {
                w = compute_content_width(e);
                w += e.style.border.left.get();
                w += e.style.border.right.get();
                w += resolve(e.style.padding.left, 0.f, [&] { return reference_parent_width(e); });
                w += resolve(e.style.padding.right, 0.f, [&] { return reference_parent_width(e); });
            }
            else // from parent
            {
                w = resolve(e.style.bounds.width, 0.f, [&] { return reference_parent_width(e); });

                if (e.style.box_sizing == style::box_type::padding_box)
                {
                    w += e.style.border.left.get();
                    w += e.style.border.right.get();
                }
            }
            e.width = w;

            CC_ASSERT(is_valid(e.width));
            return e.width;
        }

        float compute_height(layouted_element& e)
        {
            CC_ASSERT(e.height != computing && "cyclic layout detected");
            if (e.height != unassigned)
                return e.height;
            e.height = computing;

            float h = 0.f;

            // stretched
            if (e.is_absolute() && e.style.bounds.height.is_auto() && !e.style.bounds.top.is_auto() && !e.style.bounds.bottom.is_auto())
            {
                auto y = compute_y(e);

                auto margin_bottom = resolve(e.style.margin.bottom, 0.f, [&] { return reference_parent_height(e); });
                auto bottom = resolve(e.style.bounds.bottom, 0.f, [&] { return reference_parent_height(e); });
                auto max_y = reference_parent_y(e) + reference_parent_height(e) - margin_bottom - bottom;

                h = max_y - y;
            }
            else if (e.style.box_sizing == style::box_type::content_box || e.style.bounds.height.is_auto()) // from content
            {
                h = compute_content_height(e);
                h += e.style.border.top.get();
                h += e.style.border.bottom.get();
                h += resolve(e.style.padding.top, 0.f, [&] { return reference_parent_height(e); });
                h += resolve(e.style.padding.bottom, 0.f, [&] { return reference_parent_height(e); });
            }
            else // from parent
            {
                h = resolve(e.style.bounds.height, 0.f, [&] { return reference_parent_height(e); });

                if (e.style.box_sizing == style::box_type::padding_box)
                {
                    h += e.style.border.top.get();
                    h += e.style.border.bottom.get();
                }
            }
            e.height = h;

            CC_ASSERT(is_valid(e.height));
            return e.height;
        }

        float compute_content_x(layouted_element& e)
        {
            CC_ASSERT(e.content_x != computing && "cyclic layout detected");
            if (e.content_x != unassigned)
                return e.content_x;
            e.content_x = computing;

            e.content_x = compute_x(e) +              //
                          e.style.border.left.get() + //
                          resolve(e.style.padding.left, 0.f, [&] { return reference_parent_width(e); });

            CC_ASSERT(is_valid(e.content_x));
            return e.content_x;
        }

        float compute_content_y(layouted_element& e)
        {
            CC_ASSERT(e.content_y != computing && "cyclic layout detected");
            if (e.content_y != unassigned)
                return e.content_y;
            e.content_y = computing;

            e.content_y = compute_y(e) +             //
                          e.style.border.top.get() + //
                          resolve(e.style.padding.top, 0.f, [&] { return reference_parent_height(e); });

            CC_ASSERT(is_valid(e.content_y));
            return e.content_y;
        }

        float compute_content_width(layouted_element& e)
        {
            CC_ASSERT(e.content_width != computing && "cyclic layout detected");
            if (e.content_width != unassigned)
                return e.content_width;
            e.content_width = computing;

            float w = 0.f;
            if (e.style.bounds.width.is_auto()) // from children
            {
                auto const cs = e.child_start;
                auto const ce = e.child_start + e.child_count;

                switch (e.style.layout)
                {
                case style::layout::left_right:
                {
                    w = e.text_width;
                    auto last_normal = -1;
                    for (auto i = cs; i < ce; ++i)
                    {
                        auto& c = merger._layout_tree[i];
                        if (c.is_normal())
                            last_normal = i;
                    }
                    if (last_normal != -1)
                    {
                        auto& c = merger._layout_tree[last_normal];
                        w = get_right(c) + get_right_margin(c) - compute_x(e);
                    }
                }
                break;
                case style::layout::top_down:
                {
                    w = e.text_width;
                    for (auto i = cs; i < ce; ++i)
                    {
                        auto& c = merger._layout_tree[i];
                        if (c.is_normal())
                            w = tg::max(w, compute_width(c) + get_left_margin(c) + get_right_margin(c));
                    }
                }
                break;
                }
            }
            else if (e.style.box_sizing == style::box_type::content_box) // from parent
            {
                w = resolve(e.style.bounds.width, 0.f, [&] { return reference_parent_width(e); });
            }
            else // from width
            {
                w = compute_width(e);
                w -= resolve(e.style.padding.left, 0.f, [&] { return reference_parent_width(e); });
                w -= resolve(e.style.padding.right, 0.f, [&] { return reference_parent_width(e); });

                if (e.style.box_sizing == style::box_type::border_box)
                {
                    w -= e.style.border.left.get();
                    w -= e.style.border.right.get();
                }
            }
            e.content_width = w;

            CC_ASSERT(is_valid(e.content_width));
            return e.content_width;
        }

        float compute_content_height(layouted_element& e)
        {
            CC_ASSERT(e.content_height != computing && "cyclic layout detected");
            if (e.content_height != unassigned)
                return e.content_height;
            e.content_height = computing;

            float h = 0.f;
            if (e.style.bounds.height.is_auto()) // from children
            {
                auto const cs = e.child_start;
                auto const ce = e.child_start + e.child_count;

                switch (e.style.layout)
                {
                case style::layout::left_right:
                {
                    h = e.text_height;
                    for (auto i = cs; i < ce; ++i)
                    {
                        auto& c = merger._layout_tree[i];
                        if (c.is_normal())
                            h = tg::max(h, compute_height(c) + get_top_margin(c) + get_bottom_margin(c));
                    }
                }
                break;
                case style::layout::top_down:
                {
                    h = e.text_height;
                    auto last_normal = -1;
                    for (auto i = cs; i < ce; ++i)
                    {
                        auto& c = merger._layout_tree[i];
                        if (c.is_normal())
                            last_normal = i;
                    }
                    if (last_normal != -1)
                    {
                        auto& c = merger._layout_tree[last_normal];
                        h = get_bottom(c) + get_bottom_margin(c) - compute_y(e);
                    }
                }
                break;
                }
            }
            else if (e.style.box_sizing == style::box_type::content_box) // from parent
            {
                h = resolve(e.style.bounds.height, 0.f, [&] { return reference_parent_height(e); });
            }
            else // from height
            {
                h = compute_height(e);
                h -= resolve(e.style.padding.top, 0.f, [&] { return reference_parent_height(e); });
                h -= resolve(e.style.padding.bottom, 0.f, [&] { return reference_parent_height(e); });

                if (e.style.box_sizing == style::box_type::border_box)
                {
                    h -= e.style.border.top.get();
                    h -= e.style.border.bottom.get();
                }
            }
            e.content_height = h;

            CC_ASSERT(is_valid(e.content_height));
            return e.content_height;
        }

        // entry points
    public:
        void compute_all(layouted_element& e)
        {
            CC_ASSERT(e.element);

            // force layout values
            compute_x(e);
            compute_y(e);
            compute_width(e);
            compute_height(e);
            compute_content_x(e);
            compute_content_y(e);
            compute_content_width(e);
            compute_content_height(e);

            // set properties
            tree.set_property(*e.element, si::property::aabb, e.bounds());

            // recurse into children
            auto cs = e.child_start;
            auto ce = e.child_start + e.child_count;
            for (auto i = cs; i < ce; ++i)
                compute_all(merger._layout_tree[i]);
        }

        void fill_pass(layouted_element& e)
        {
            if (e.style.bounds.fill_width)
            {
                auto p_right = e.is_root() ? merger.viewport.max.x : get_content_right(parent_of(e));
                auto e_right = e.x + e.width + e.style.margin.right.get(); // TODO: box sizing?
                if (e_right < p_right)
                {
                    auto dw = p_right - e_right;
                    e.width += dw;
                    e.content_width += dw;

                    // TODO: adapt everything that depends on parent width?
                }
            }

            if (e.style.bounds.fill_height)
            {
                CC_ASSERT(false && "not implemented");
            }

            // recurse into children
            auto cs = e.child_start;
            auto ce = e.child_start + e.child_count;
            for (auto i = cs; i < ce; ++i)
                fill_pass(merger._layout_tree[i]);
        }

        void on_after_layout(layouted_element& le)
        {
            // text align
            if (le.has_text)
            {
                // TODO: vertical align?
                switch (le.style.font.align)
                {
                case style::font_align::left:
                    le.text_origin = {le.content_x, le.content_y};
                    break;
                case style::font_align::center:
                    le.text_origin = {le.content_x + le.width / 2 - le.text_width / 2, le.content_y};
                    break;
                case style::font_align::right:
                    le.text_origin = {le.content_x + le.width - le.text_width, le.content_y};
                    break;
                }
            }

            // text edit
            auto& e = *le.element;
            if (e.type == element_type::input && tree.get_property_or(e, si::property::edit_text, false))
            {
                merger._is_in_text_edit = true;
                auto txt = tree.get_property(e, si::property::text);

                // set editable text on first edit
                // TODO: select all or update cursor
                auto pe = merger._prev_ui->get_element_by_id(e.id);
                auto prev_edit = merger._prev_ui->get_property_or(pe, si::property::edit_text, false);
                if (!prev_edit)
                {
                    merger._editable_text.reset(txt);
                    merger._editable_text.select_all();
                }

                // TODO: selection

                // sync text property with editable text
                // TODO: only if composition finished
                if (merger._editable_text.text() != txt)
                    tree.set_property(e, si::property::text, merger._editable_text.text());

                // NOTE: only works for non-special types currently
                // TODO: align?
                merger.set_editable_text_glyphs(txt, le.x, le.y, le.style.font);
                le.is_in_text_edit = true;
            }

            // recurse into children
            auto cs = le.child_start;
            auto ce = le.child_start + le.child_count;
            for (auto i = cs; i < ce; ++i)
                on_after_layout(merger._layout_tree[i]);
        }
    };

    auto layouter = layouter_t(tree, *this);

    // force computation of all layout values
    for (auto i : _layout_roots)
        layouter.compute_all(_layout_tree[i]);

    // post-process fill
    for (auto i : _layout_roots)
        layouter.fill_pass(_layout_tree[i]);

    // resolve style values
    for (auto i : _layout_roots)
        layouter.on_after_layout(_layout_tree[i]);
}
