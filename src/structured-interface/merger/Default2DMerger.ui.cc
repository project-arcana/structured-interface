#include "Default2DMerger.hh"

#include <reflector/to_string.hh>

#include <structured-interface/si.hh>

void si::Default2DMerger::show_stats_ui(bool use_window)
{
    auto build_ui = [&] {
        // TODO: collapsible groups? indent?

        {
            si::text("input:");
            si::text("  mouse pos: {}, {}", mouse_pos.x, mouse_pos.y);
            si::text("  is_lmb_down: {}", is_lmb_down);
            si::text("  drag_distance: {}", drag_distance);
        }

        si::text("");

        {
            auto const& rd = get_render_data();
            si::text("render data:");
            si::text("  lists: {}", rd.lists.size());
            for (auto const& l : rd.lists)
            {
                si::text("  vertices: {}", l.vertices.size());
                si::text("  indices: {}", l.indices.size());
                si::text("  cmds: {}", l.cmds.size());
            }
        }

        si::text("");

        {
            si::text("layout data:");
            si::text("  nodes: {}", _layout_tree.size());
            si::text("  roots: {}", _layout_roots.size());
            si::text("  deferred placements: {}", _deferred_placements.size());
        }

        si::text("");

        {
            si::text("style data:");
            si::text("  rules: {}", _stylesheet.get_style_rule_count());
            si::text("  cached styles: {}", _stylesheet.get_cached_styles_count());
        }
    };

    if (use_window)
    {
        if (auto w = si::window("Default2DMerger Stats"))
            build_ui();
    }
    else
        build_ui();
}

void si::Default2DMerger::show_inspector_ui()
{
    // TODO: only id, must be refetched each frame!
    static struct
    {
        bool valid = false;
        layouted_element layout; // CAUTION: element pointer is invalid
    } curr;

    if (auto w = si::window("si::inspector"))
    {
        if (si::button("drag me").is_pressed())
        {
            if (auto e = query_layout_element_at(mouse_pos))
            {
                curr.valid = true;
                curr.layout = *e;
            }
            else
                curr.valid = false;
        }

        if (curr.valid)
        {
            si::text("bounds: {}", rf::to_string(curr.layout.bounds));
            si::text("content_start: {}", rf::to_string(curr.layout.content_start));
        }
    }
}
