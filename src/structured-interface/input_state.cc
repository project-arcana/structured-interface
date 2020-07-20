#include "input_state.hh"

void si::input_state::on_next_update()
{
    hover_last = hover_curr;
    hover_curr = {};

    pressed_last = pressed_curr;
    pressed_curr = {};
}
