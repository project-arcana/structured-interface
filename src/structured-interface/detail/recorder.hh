#pragma once

#include <structured-interface/gui.hh>

#include <clean-core/typedefs.hh>
#include <clean-core/vector.hh>

namespace si::detail
{
void start_recording(gui const& ui);
void end_recording(cc::vector<std::byte>& data);
}
