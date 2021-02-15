#include "si.hh"

#include <typed-geometry/tg.hh>

#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_tree.hh>

using style_entry = si::style::style_entry;

namespace si::detail
{
static bool is_or_was_disabled(element_handle id)
{
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->get_element_by_id(id);

    if (!e) // element first created? pretend to be disabled (to prevent false-positive clicks)
        return true;

    while (e)
    {
        // element or parent is disabled
        if (!ui->get_property_or(*e, si::property::enabled, true))
            return true;

        e = ui->parent_of(*e);
    }

    // element chain is enabled
    return false;
}
}

void si::ui_element_base::set_style_class(uint16_t class_id) { si::detail::write_property(id, si::property::style_class, class_id); }

void si::ui_element_base::set_left(float px) { si::detail::write_property(id, si::property::style_value, {style_entry::left_abs, px}); }
void si::ui_element_base::set_left_relative(float v) { si::detail::write_property(id, si::property::style_value, {style_entry::left_rel, v}); }
void si::ui_element_base::set_top(float px) { si::detail::write_property(id, si::property::style_value, {style_entry::top_abs, px}); }
void si::ui_element_base::set_top_relative(float v) { si::detail::write_property(id, si::property::style_value, {style_entry::top_rel, v}); }
void si::ui_element_base::set_width(float px) { si::detail::write_property(id, si::property::style_value, {style_entry::width_abs, px}); }
void si::ui_element_base::set_width_relative(float v) { si::detail::write_property(id, si::property::style_value, {style_entry::width_rel, v}); }
void si::ui_element_base::set_height(float px) { si::detail::write_property(id, si::property::style_value, {style_entry::height_abs, px}); }
void si::ui_element_base::set_height_relative(float v) { si::detail::write_property(id, si::property::style_value, {style_entry::height_rel, v}); }

void si::ui_element_base::set_enabled(bool enabled)
{
    si::detail::write_property(id, si::property::enabled, enabled);
    _is_enabled = enabled;
}

bool si::ui_element_base::is_or_was_disabled() const
{
    if (!_is_enabled)
        return true;

    return detail::is_or_was_disabled(id);
}

si::button_t si::button(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::button, text);
    si::detail::write_property(id, si::property::text, text);
    return {id};
}

si::button_t si::button(cc::string_view text, bool is_enabled)
{
    auto id = si::detail::start_element(element_type::button, text);
    si::detail::write_property(id, si::property::text, text);
    return {id, is_enabled};
}

si::checkbox_t si::checkbox(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::checkbox, text);
    si::detail::write_property(id, si::property::text, text);

    auto changed = false;
    if (detail::current_input_state().was_clicked(id) && !detail::is_or_was_disabled(id))
    {
        changed = true;
        ok = !ok; // toggle on click
    }

    // to style the checkbox
    if (auto b = si::box())
        si::detail::write_property(b.id, si::property::no_input, true); // whole checkbox is clickable

    si::detail::write_property(id, si::property::state_u8, uint8_t(ok));
    return {id, changed};
}

cc::pair<si::element_handle, bool> si::detail::impl_radio_button(cc::string_view text, bool active)
{
    auto id = si::detail::start_element(element_type::radio_button, text);
    si::detail::write_property(id, si::property::text, text);

    auto changed = false;
    if (detail::current_input_state().was_clicked(id) && !detail::is_or_was_disabled(id))
    {
        changed = true;
        active = true;
    }

    // to style the radio_button
    if (auto b = si::box())
        si::detail::write_property(b.id, si::property::no_input, true); // whole radio_button is clickable

    si::detail::write_property(id, si::property::state_u8, uint8_t(active));
    return {id, changed};
}

si::toggle_t si::toggle(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::toggle, text);
    si::detail::write_property(id, si::property::text, text);

    auto changed = false;
    if (detail::current_input_state().was_clicked(id) && !detail::is_or_was_disabled(id))
    {
        changed = true;
        ok = !ok; // toggle on click
    }

    // to style the toggle
    if (auto b = si::box())
        si::detail::write_property(b.id, si::property::no_input, true); // whole toggle is clickable

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

    if (io.is_pressed(id) && !detail::is_or_was_disabled(id)) // is LMB pressed
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
        b.set_left_relative(t);
        si::detail::write_property(b.id, si::property::no_input, true);
    }

    return {id, changed};
}

namespace si
{
static si::window_t impl_window(cc::string_view title, bool* visible)
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
}

si::window_t si::window(cc::string_view title) { return impl_window(title, nullptr); }
si::window_t si::window(cc::string_view title, bool& visible) { return impl_window(title, &visible); }

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

si::collapsible_group_t si::collapsible_group(cc::string_view text, cc::flags<collapsible_group_options> options)
{
    auto id = si::detail::start_element(element_type::collapsible_group, text);

    // heading
    auto cid = si::detail::start_element(element_type::heading, text);
    si::detail::write_property(cid, si::property::text, text);
    si::detail::end_element(cid);

    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;
    auto e = ui->get_element_by_id(id);

    auto collapsed = ui->get_property_or(e, si::property::collapsed, options.has(collapsible_group_options::start_collapsed));

    if (io.was_clicked(cid))
        collapsed = !collapsed;
    si::detail::write_property(id, si::property::collapsed, collapsed);

    return {id, !collapsed};
}

si::scroll_area_t si::scroll_area()
{
    auto const& io = detail::current_input_state();
    auto ui = si::detail::current_ui_context().prev_ui;

    auto id = si::detail::start_element(element_type::scroll_area); // TODO: custom id

    auto box_id = si::detail::start_element(element_type::box);
    auto box_e = ui->get_element_by_id(box_id);

    // use abs pos as scroll pos
    tg::pos2 abs_pos;
    if (ui->get_property_to(box_e, si::property::absolute_pos, abs_pos))
    {
        if (io.scroll_delta.y != 0 && io.is_hovered(id))
        {
            // TODO: content aabb
            auto e = ui->get_element_by_id(id);

            tg::aabb2 parent_bb;
            tg::aabb2 bb;
            if (ui->get_property_to(e, si::property::aabb, parent_bb) && ui->get_property_to(box_e, si::property::aabb, bb))
            {
                abs_pos.y += io.scroll_delta.y * 100; // TODO: how much

                auto max_scroll = tg::size_of(bb) - tg::size_of(parent_bb);

                if (max_scroll.width <= 0)
                    abs_pos.x = 0;
                else
                    abs_pos.x = tg::clamp(abs_pos.x, -max_scroll.width, 0.f);

                if (max_scroll.height <= 0)
                    abs_pos.y = 0;
                else
                    abs_pos.y = tg::clamp(abs_pos.y, -max_scroll.height, 0.f);
            }
        }
    }

    // always written, zero-init
    si::detail::write_property(box_id, si::property::absolute_pos, abs_pos);

    return {id, box_id};
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
