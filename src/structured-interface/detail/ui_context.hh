#pragma once

#include <clean-core/assert.hh>

#include <structured-interface/fwd.hh>

namespace si::detail
{
/// a context object for ui_elements
/// used for queries
struct ui_context
{
    input_state* input = nullptr;
    element_tree const* prev_ui = nullptr;
};

/// returns a thread_local context object
/// TODO: maybe it's faster to make a thread_local pointer instead
inline ui_context& current_ui_context()
{
    thread_local ui_context ctx;
    return ctx;
}

/// returns the current input state
/// CAUTION: only usable withing gui::record
inline input_state const& current_input_state()
{
    auto input = current_ui_context().input;
    CC_ASSERT(input && "this function is only valid withing gui::record");
    return *input;
}
}
