#include "si.hh"


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
