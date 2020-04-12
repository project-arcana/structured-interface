#include "gui.hh"

#include <structured-interface/detail/recorder.hh>

si::recorded_ui si::gui::record(cc::function_ref<void()> do_record)
{
    si::detail::start_recording(*this);

    // call user UI recorder
    do_record();

    // TODO: less copying
    cc::vector<std::byte> data;
    si::detail::end_recording(data);
    return recorded_ui(cc::move(data));
}

bool si::gui::has(cc::string_view name) const
{
    // TODO
    return false;
}
