#include "input_state.hh"

void si::input_state::on_next_update()
{
    direct_hover_last = direct_hover_curr;
    direct_hover_curr = {};

    // TODO: better memory reuse?
    hovers_last = cc::move(hovers_curr);
    hovers_curr.clear();

    focus_last = focus_curr;
    focus_curr = {};

    pressed_last = pressed_curr;
    pressed_curr = {};

    clicked_curr = {};
}
