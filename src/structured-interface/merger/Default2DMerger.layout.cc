#include "Default2DMerger.hh"

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

            switch (e.style.box_child_ref)
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

            switch (e.style.box_child_ref)
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

        // main layout computations
    public:
        float compute_x(layouted_element& e)
        {
            CC_ASSERT(e.x != computing && "cyclic layout detected");
            if (e.x != unassigned)
                return e.x;
            e.x = computing;

            auto margin_left = resolve(e.style.margin.left, 0.f, [&] { return reference_parent_width(e); });

            // TODO: centered if margin left/right is auto and width is non-auto for absolute

            // left is only relative to sibling for normal non-root left-right layouts
            if (!e.is_absolute() && e.has_prev_normal_sibling() && is_parent_left_right(e))
            {
                // TODO: use margin right from prev? -> collapse
                e.x = compute_x(merger._layout_tree[e.prev_normal_idx]) + margin_left;
            }
            else
            {
                e.x = reference_parent_x(e) + margin_left;
            }

            CC_ASSERT(e.x != computing);
            return e.x;
        }

        float compute_y(layouted_element& e)
        {
            CC_ASSERT(e.y != computing && "cyclic layout detected");
            if (e.y != unassigned)
                return e.y;
            e.y = computing;

            auto margin_top = resolve(e.style.margin.top, 0.f, [&] { return reference_parent_height(e); });

            // TODO: centered if margin top/bottom is auto and height is non-auto for absolute

            // left is only relative to sibling for normal non-root left-right layouts
            if (!e.is_absolute() && e.has_prev_normal_sibling() && is_parent_top_down(e))
            {
                // TODO: use margin right from prev? -> collapse
                e.y = compute_y(merger._layout_tree[e.prev_normal_idx]) + margin_top;
            }
            else
            {
                e.y = reference_parent_y(e) + margin_top;
            }

            CC_ASSERT(e.y != computing);
            return e.y;
        }

        float compute_width(layouted_element& e)
        {
            CC_ASSERT(e.width != computing && "cyclic layout detected");
            if (e.width != unassigned)
                return e.width;
            e.width = computing;

            float w = 0.f;
            if (e.style.box_sizing == style::box_type::content_box || e.style.bounds.width.is_auto()) // from content
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

            CC_ASSERT(e.width != computing);
            return e.width;
        }

        float compute_height(layouted_element& e)
        {
            CC_ASSERT(e.height != computing && "cyclic layout detected");
            if (e.height != unassigned)
                return e.height;
            e.height = computing;

            float h = 0.f;
            if (e.style.box_sizing == style::box_type::content_box || e.style.bounds.height.is_auto()) // from content
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

            CC_ASSERT(e.height != computing);
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

            CC_ASSERT(e.content_x != computing);
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

            CC_ASSERT(e.content_y != computing);
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
                // TODO: also text stuff

                // TODO
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

            CC_ASSERT(e.content_width != computing);
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
                // TODO: also text stuff

                // TODO
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

            CC_ASSERT(e.content_height != computing);
            return e.content_height;
        }

        // entry points
    public:
        void compute_all(layouted_element& e)
        {
            // force layout values
            compute_x(e);
            compute_y(e);
            compute_width(e);
            compute_height(e);
            compute_content_x(e);
            compute_content_y(e);
            compute_content_width(e);
            compute_content_height(e);

            // recurse into children
            auto cs = e.child_start;
            auto ce = e.child_start + e.child_count;
            for (auto i = cs; i < ce; ++i)
                compute_all(merger._layout_tree[i]);
        }

        void resolve_style(layouted_element& e)
        {
            // TODO: border values?
        }
    };

    auto layouter = layouter_t(tree, *this);

    // force computation of all layout values
    for (auto i : _layout_roots)
        layouter.compute_all(_layout_tree[i]);

    // resolve style values
    for (auto i : _layout_roots)
        layouter.resolve_style(_layout_tree[i]);
}
