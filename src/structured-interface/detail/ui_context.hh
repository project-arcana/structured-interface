#pragma once

#include <structured-interface/fwd.hh>

namespace si::detail
{
/// a context object for ui_elements
/// used for queries
struct ui_context
{
    input_state* input = nullptr;
};

/// returns a thread_local context object
/// TODO: maybe it's faster to make a thread_local pointer instead
inline ui_context& current_ui_context()
{
    thread_local ui_context ctx;
    return ctx;
}
}
