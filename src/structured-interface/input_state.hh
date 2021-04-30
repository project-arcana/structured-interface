#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/handles.hh>

namespace si
{
/// a helper struct that contains the input state of an element_tree
/// used for answering user-space queries such as "is_hovered" or "was_clicked"
/// (without querying into the element_tree directly)
///
/// TODO: is_hovered during record can be optimized by tracking the hover stack
struct input_state
{
    element_handle direct_hover_last;
    element_handle direct_hover_curr;

    element_handle focus_last;
    element_handle focus_curr;

    // hover stack
    // NOTE: topmost element is first
    // NOTE: currently ignores detached, i.e. follows the recorded ui structure
    cc::vector<element_handle> hovers_last;
    cc::vector<element_handle> hovers_curr;

    // TODO: invalid pressed when released outside?
    element_handle pressed_last;
    element_handle pressed_curr;
    bool is_drag = false; // TODO: also a timed component?

    element_handle clicked_curr; // no last

    // NOTE: in some mergers, this doesn't make much sense
    tg::pos2 mouse_pos;
    tg::vec2 mouse_delta;
    tg::vec2 scroll_delta;

    // sets "last" to "curr" and "curr" to empty
    void on_next_update();

    // common queries
public:
    bool was_clicked(element_handle id) const { return clicked_curr == id; }
    bool is_hovered(element_handle id) const { return hovers_curr.contains(id); }
    bool is_hover_entered(element_handle id) const { return hovers_curr.contains(id) && !hovers_last.contains(id); }
    bool is_hover_left(element_handle id) const { return !hovers_curr.contains(id) && hovers_last.contains(id); }
    bool is_direct_hovered(element_handle id) const { return direct_hover_curr == id; }
    bool is_direct_hover_entered(element_handle id) const { return direct_hover_curr == id && direct_hover_last != id; }
    bool is_direct_hover_left(element_handle id) const { return direct_hover_curr != id && direct_hover_last == id; }
    bool is_pressed(element_handle id) const { return pressed_curr == id; }
    bool is_focused(element_handle id) const { return focus_curr == id; }
    bool is_focus_gained(element_handle id) const { return focus_curr == id && focus_last != id; }
    bool is_focus_lost(element_handle id) const { return focus_curr != id && focus_last == id; }
    bool is_dragged(element_handle id) const { return pressed_curr == id && is_drag; }
};
}
