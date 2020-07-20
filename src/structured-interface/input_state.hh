#pragma once

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

    // sets "last" to "curr" and "curr" to empty
    void on_next_update();
};
}
