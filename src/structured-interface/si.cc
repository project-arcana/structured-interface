#include "si.hh"

#include <rich-log/log.hh> // DEBUG

#include <typed-geometry/tg.hh>

#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_tree.hh>

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

    si::box(); // to style the checkbox

    si::detail::write_property(id, si::property::state_u8, uint8_t(ok));
    return {id, changed};
}

si::toggle_t si::toggle(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::toggle, text);
    si::detail::write_property(id, si::property::text, text);

    auto changed = false;
    if (detail::current_input_state().was_clicked(id))
    {
        changed = true;
        ok = !ok; // toggle on click
    }

    si::box(); // to style the toggle

    si::detail::write_property(id, si::property::state_u8, uint8_t(ok));
    return {id, changed};
}

si::textbox_t si::textbox(cc::string_view desc, cc::string& value)
{
    auto id = si::detail::start_element(element_type::textbox, desc);
    si::detail::write_property(id, si::property::text, desc);

    auto const& io = detail::current_input_state();
    auto changed = false;

    auto cid = si::detail::start_element(element_type::input, desc);
    si::detail::write_property(cid, si::property::no_input, true); // focus is handled via textbox

    // can edit if focused
    if (io.is_focused(id))
    {
        auto ui = si::detail::current_ui_context().prev_ui;
        auto ce = ui->get_element_by_id(cid);
        cc::string_view prev_value;
        if (ui->get_property_to(ce, si::property::text, prev_value))
            if (prev_value != value) // text is edited
            {
                changed = true;
                value = prev_value;
            }
        si::detail::write_property(cid, si::property::edit_text, true);
    }
    si::detail::write_property(cid, si::property::text, cc::string_view(value));
    si::detail::end_element(cid);

    return {id, changed};
}

si::text_t si::text(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::text, text);
    si::detail::write_property(id, si::property::text, text);
    return {id};
}

si::heading_t si::heading(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::heading, text);
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
    bool changed = false;

    if (io.is_pressed(id)) // is LMB pressed
    {
        auto ui = si::detail::current_ui_context().prev_ui;
        auto e = ui->get_element_by_id(id);
        if (e)
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
    }

    si::detail::write_property(id, si::property::state_f32, t);

    // knob
    if (auto b = si::box())
    {
        si::detail::write_property(b.id, si::property::detail::left_percentage, t);
        si::detail::write_property(b.id, si::property::no_input, true);
    }

    return {id, changed};
}

si::window_t si::window(cc::string_view title)
{
    auto id = si::detail::start_element(element_type::window, title);

    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->get_element_by_id(id);

    tg::vec2 pos_delta;

    auto collapsed = ui->get_property_or(e, si::property::collapsed, false);

    // TODO: drag behavior

    // title area
    {
        auto h = si::heading(title);

        if (h.was_clicked())
            collapsed = !collapsed; // TODO: only if "true" click?

        if (h.is_dragged())
            pos_delta = io.mouse_delta;
    }

    // abs pos
    tg::pos2 abs_pos;
    if (ui->get_property_to(e, si::property::absolute_pos, abs_pos))
    {
        abs_pos += pos_delta;
        si::detail::write_property(id, si::property::absolute_pos, abs_pos);
    }

    // state
    si::detail::write_property(id, si::property::collapsed, collapsed);

    return {id, !collapsed};
}

si::tooltip_t si::tooltip(placement placement)
{
    auto pid = si::detail::curr_element();
    si::element_handle id;
    bool visible = false;

    auto const& io = detail::current_input_state();
    if (io.is_hovered(pid))
    {
        id = si::detail::start_element(element_type::tooltip); // TODO: custom id?

        si::detail::write_property(id, si::property::detached, true);
        si::detail::write_property(id, si::property::placement, placement);

        visible = true;
    }

    return {id, visible};
}

si::popover_t si::popover(si::placement placement)
{
    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;

    auto pid = si::detail::curr_element();
    auto id = si::detail::start_element(element_type::popover); // TODO: custom id?

    si::detail::write_property(id, si::property::detached, true);
    si::detail::write_property(id, si::property::placement, placement);

    auto e = ui->get_element_by_id(id);
    auto vis = ui->get_property_or(e, si::property::visibility, si::style::visibility::none);

    // toggle on click
    if (io.was_clicked(pid))
        vis = vis == si::style::visibility::visible ? si::style::visibility::none : si::style::visibility::visible;

    si::detail::write_property(id, si::property::visibility, vis);

    return {id, vis == si::style::visibility::visible};
}

si::row_t si::row()
{
    auto id = si::detail::start_element(element_type::row);
    return {id, true};
}

si::canvas_t si::canvas(tg::size2 size)
{
    auto id = si::detail::start_element(element_type::canvas);
    si::detail::write_property(id, si::property::fixed_size, size);
    return {id, true};
}

si::canvas_t::~canvas_t()
{
    CC_ASSERT(id.is_valid() && "cannot finish canvas early currently");
    si::detail::write_property(id, si::property::custom_triangles, cc::span<colored_vertex>(_triangles));
}

si::box_t si::box()
{
    auto id = si::detail::start_element(element_type::box);
    return {id, true};
}

si::separator_t si::separator()
{
    auto id = si::detail::start_element(element_type::separator);
    return {id};
}

si::spacing_t si::spacing(float size)
{
    auto id = si::detail::start_element(element_type::spacing);
    si::detail::write_property(id, si::property::fixed_size, {0.f, size});
    return {id};
}
