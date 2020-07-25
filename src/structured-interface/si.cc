#include "si.hh"

#include <typed-geometry/tg.hh>

#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_tree.hh>

namespace
{
tg::aabb2 query_aabb(si::element_handle id)
{
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->element_by_id(id);
    return e ? ui->get_property_or(*e, si::property::aabb, tg::aabb2()) : tg::aabb2();
}
}

tg::aabb2 si::ui_element_base::aabb() const { return query_aabb(id); }

si::button_t si::button(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::button, text);
    si::detail::write_property(id, si::property::text, text);
    return {id};
}

si::checkbox_t si::checkbox(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::checkbox, text);
    si::detail::write_property(id, si::property::text, text);

    auto changed = false;
    if (detail::current_input_state().was_clicked(id))
    {
        changed = true;
        ok = !ok; // toggle on click
    }

    si::detail::write_property(id, si::property::state_u8, uint8_t(ok));
    return {id, changed};
}

si::text_t si::text(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::text, text);
    si::detail::write_property(id, si::property::text, text);
    return {id};
}

si::clickable_area_t si::clickable_area()
{
    auto id = si::detail::start_element(element_type::clickable_area);
    return {id};
}

si::slider_area_t si::slider_area(float& t)
{
    auto id = si::detail::start_element(element_type::slider_area);

    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->element_by_id(id);
    bool changed = false;

    if (e && io.is_pressed(id)) // is LMB pressed
    {
        auto bb = ui->get_property_or(*e, si::property::aabb, tg::aabb2());
        auto prev_t = ui->get_property_or(*e, si::property::state_f32, t);
        auto new_t = prev_t;
        auto w = bb.max.x - bb.min.x;
        if (w > 0) // well defined width
        {
            auto mx = tg::clamp(io.mouse_pos.x, bb.min.x, bb.max.x);
            new_t = (mx - bb.min.x) / float(w);
            CC_ASSERT(0 <= new_t && new_t <= 1);
        }

        if (t != new_t)
        {
            t = new_t;
            changed = true;
        }
    }

    si::detail::write_property(id, si::property::state_f32, t);
    return {id, changed};
}

si::window_t si::window(cc::string_view title)
{
    auto id = si::detail::start_element(element_type::window, title);
    si::detail::write_property(id, si::property::text, title);

    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->element_by_id(id);

    tg::vec2 pos_delta;

    auto collapsed = ui->get_property_or(e, si::property::collapsed, false);

    // TODO: drag behavior

    // title area
    {
        auto c = si::clickable_area();

        if (c.was_clicked())
            collapsed = !collapsed; // TODO: only if "true" click?

        if (c.is_dragged())
            pos_delta = io.mouse_delta;

        // TODO: toggle collapse
    }

    // abs pos
    tg::pos2 abs_pos;
    if (e && ui->get_property_to(*e, si::property::absolute_pos, abs_pos))
    {
        abs_pos += pos_delta;
        si::detail::write_property(id, si::property::absolute_pos, abs_pos);
    }

    // state
    si::detail::write_property(id, si::property::collapsed, collapsed);

    return {id, !collapsed};
}

void si::detail::make_tooltip(si::element_handle id, cc::string_view text)
{
    auto const& io = detail::current_input_state();

    if (!io.is_hovered(id))
        return; // TODO: also for nested / children

    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->element_by_id(id);

    tg::aabb2 bb;
    if (!ui->get_property_to(e, si::property::aabb, bb))
        return; // no aabb yet

    auto t = si::text(text);
    tg::aabb2 bbt;
    ui->get_property_to(ui->element_by_id(t.id), si::property::aabb, bbt);
    si::detail::write_property(id, si::property::detached, true);
    // TODO: make a relative constraint property to position this
    si::detail::write_property(id, si::property::absolute_pos, bb.min - tg::vec2(0, 4 + tg::size_of(bbt).height));
}
