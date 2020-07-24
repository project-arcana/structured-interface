#pragma once

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/handles.hh>

namespace si
{
/// a helper struct that contains the input state of an element_tree
/// used for answering user-space queries such as "is_hovered" or "was_clicked"
/// (without querying into the element_tree directly)
struct input_state
{
    element_handle hover_last;
    element_handle hover_curr;

    // TODO: invalid pressed when released outside?
    element_handle pressed_last;
    element_handle pressed_curr;
    bool is_drag = false; // TODO: also a timed component?

    // NOTE: in some mergers, this doesn't make much sense
    tg::pos2 mouse_pos;
    tg::vec2 mouse_delta;

    // sets "last" to "curr" and "curr" to empty
    void on_next_update();

    // common queries
public:
    bool was_clicked(element_handle id) const { return !is_drag && pressed_last == id && pressed_curr != id; }
    bool is_hovered(element_handle id) const { return hover_curr == id; }
    bool is_pressed(element_handle id) const { return pressed_curr == id; }
    bool is_dragged(element_handle id) const { return pressed_curr == id && is_drag; }
};
}
