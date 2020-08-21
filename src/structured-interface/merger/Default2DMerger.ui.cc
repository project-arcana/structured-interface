#include "Default2DMerger.hh"

#include <typed-geometry/tg.hh>

#include <reflector/to_string.hh>

#include <structured-interface/element_tree.hh>
#include <structured-interface/si.hh>

void si::Default2DMerger::show_stats_ui(bool use_window)
{
    auto build_ui = [&] {
        if (auto h = si::collapsible_group("timings"))
        {
            if (_seconds_record > 0)
                si::text("recording:   {} ms", tg::round(_seconds_record * 1000 * 10) / 10);
            si::text("styling:     {} ms", tg::round(_seconds_style * 1000 * 10) / 10);
            si::text("layouting:   {} ms", tg::round(_seconds_layout * 1000 * 10) / 10);
            si::text("input:       {} ms", tg::round(_seconds_input * 1000 * 10) / 10);
            si::text("render data: {} ms", tg::round(_seconds_render_data * 100 * 10) / 10);
        }

        if (auto h = si::collapsible_group("input"))
        {
            si::text("mouse pos: {}, {}", mouse_pos.x, mouse_pos.y);
            si::text("is_lmb_down: {}", is_lmb_down);
            si::text("drag_distance: {}", drag_distance);
        }

        if (auto h = si::collapsible_group("render data"))
        {
            auto const& rd = get_render_data();
            si::text("lists: {}", rd.lists.size());
            for (auto const& l : rd.lists)
            {
                si::text("vertices: {}", l.vertices.size());
                si::text("indices: {}", l.indices.size());
                si::text("cmds: {}", l.cmds.size());
            }
        }

        if (auto h = si::collapsible_group("layout data"))
        {
            si::text("layout data:");
            si::text("nodes: {}", _layout_tree.size());
            si::text("roots: {}", _layout_roots.size());
            si::text("deferred placements: {}", _deferred_placements.size());
        }

        if (auto h = si::collapsible_group("style data"))
        {
            si::text("style data:");
            si::text("rules: {}", _stylesheet.get_style_rule_count());
            si::text("cached styles: {}", _stylesheet.get_cached_styles_count());
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

void si::Default2DMerger::show_inspector_ui(si::element_tree const& ui)
{
    // only id, must be refetched each frame!
    static element_handle curr_id;

    if (auto w = si::window("si::inspector"))
    {
        if (si::button("drag me").is_pressed())
        {
            if (auto e = query_layout_element_at(mouse_pos))
            {
                curr_id = e->element->id;
            }
            else
                curr_id = {};
        }

        if (curr_id.is_valid())
        {
            si::text("id: {}", curr_id.id());
            if (auto e = ui.get_element_by_id(curr_id))
            {
                si::text("type: {}", to_string(e->type));
                si::text("children: {} (start at {})", e->children_count, e->children_start);
                si::text("properties: {} (start at {})", e->properties_count, e->properties_start);

                for (auto const& le : _layout_tree)
                    if (le.element && le.element->id == curr_id)
                    {
                        si::text("bounds: {}", rf::to_string(le.bounds));
                        si::text("content_start: {}", rf::to_string(le.content_start));
                    }
            }
            else
                si::text("[element not found]");
        }
        else
            si::text("[no element selected]");
    }
}
