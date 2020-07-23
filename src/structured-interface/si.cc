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
