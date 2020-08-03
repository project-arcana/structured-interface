#include "Simple2DMerger.hh"

#include <structured-interface/si.hh>

void si::Simple2DMerger::show_stats_ui(bool use_window)
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
            si::text("  rules: {}", _style_cache.get_style_rule_count());
            si::text("  cached styles: {}", _style_cache.get_cached_styles_count());
        }
    };

    if (use_window)
    {
        if (auto w = si::window("Simple2DMerger Stats"))
            build_ui();
    }
    else
        build_ui();
}
